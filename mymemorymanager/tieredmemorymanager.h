#ifndef TIERED_SLAB_ALLOCATOR_H_
#define TIERED_SLAB_ALLOCATOR_H_

#include <array>
#include <algorithm>	// max

#include <cstring>
#include <cassert>

#include "slaballocator.h"

#include "memorymanager_datacopy.h"

#include "logger.h"

namespace memory_manager{

	namespace memory_manager_impl_{

		constexpr uint32_t KB = 1024;

		constexpr std::array<uint32_t, 4> SIZES = {
			  16 * KB,
			  64 * KB,
			 256 * KB,
			1024 * KB
		};

		template<uint8_t tier>
		constexpr uint32_t calcMinimumCount(uint32_t value, uint32_t maxClients___) noexcept{
			constexpr uint32_t MAX		= std::numeric_limits<uint32_t>::max();
			constexpr uint32_t MIN_COUNT	= 2;

			auto const maxClients = std::min(maxClients___, MAX - 2);

			if (value == MAX){
				// undocumented, we **REALLY** know what we do :)
				return 0;
			}

			if (value){
				// we know what we do
				return std::min(value, maxClients);
			}

			switch(tier){
			case 0: return maxClients + 1;
			case 1: return std::max(maxClients /  4, MIN_COUNT);
			case 2: return std::max(maxClients / 16, MIN_COUNT);
			case 3: return 512 + 1;	// see max_packet_size from db_net_main.cc
			}

			__builtin_unreachable();
		}

	} // namespace memory_manager_impl_



	template<typename MemoryResource>
	class TieredMemoryManager{
		constexpr static auto SIZE = memory_manager_impl_::SIZES.size();

		constexpr static bool DEBUG_ALLOCATIONS		= true;
		constexpr static auto DEBUG_ALLOCATIONS_LEVEL	= Logger::Level::DEBUG;

	public:
		using size_type	= uint32_t;
		using Counts	= std::array<size_type, SIZE>;

		struct Node{
			constexpr static bool SUPPORT_CHAIN = true;

			Node		*next		; // 8
			size_type	size		; // 4
			size_type	capacity	; // 4
			size_type	head		; // 4
			uint16_t	tag		; // 2
			uint8_t		tier		; // 1
			char		padding[1]	; // 1

			char		data[4]		; // 1 + 3 pad
		};

		using NodePtr		= Node *;

		constexpr static size_t NODE_SIZE = offsetof(Node, data);

		constexpr static std::string_view getName() noexcept{
			return "TieredMemoryManager";
		}

	public:
		TieredMemoryManager(Counts const &counts, uint32_t maxClients = 0) noexcept :
			res_(
				cSize__<0>(counts, maxClients) +
				cSize__<1>(counts, maxClients) +
				cSize__<2>(counts, maxClients) +
				cSize__<3>(counts, maxClients)
			),

			slab_0_( cPtr__<0>(counts, maxClients, res_), cSize__<0>(counts, maxClients), MyBuffer::from_bytes{} ),
			slab_1_( cPtr__<1>(counts, maxClients, res_), cSize__<1>(counts, maxClients), MyBuffer::from_bytes{} ),
			slab_2_( cPtr__<2>(counts, maxClients, res_), cSize__<2>(counts, maxClients), MyBuffer::from_bytes{} ),
			slab_3_( cPtr__<3>(counts, maxClients, res_), cSize__<3>(counts, maxClients), MyBuffer::from_bytes{} ){}

		TieredMemoryManager(uint32_t maxClients = 0) noexcept :
					TieredMemoryManager( { /* zeroes */ }, maxClients){}

		~TieredMemoryManager() noexcept = default;

	public:
		template<bool Log = true>
		bool reallocate(NodePtr &node, size_t needed, uint16_t tag = 0) noexcept{
			if (needed > slab_back_().BLOCK_SIZE - NODE_SIZE)
			    return false;

			size_t const size = needed + NODE_SIZE;

			auto tier = findTier__(size);

			if (node && node->tier >= tier)
				return true;

			if (tier >= slab_size__)
				return false;

			void *p = nullptr;

			for(; tier < slab_size__; ++tier)
				if (p = allocate_(tier); p)
					break;

			if (!p)
				return false;

			if (!node){
				// case 1. create new node
				node = createNode__(p, tier, tag);
			}else{
				assert(!node->next && "reallocate() can not handle chains");

				// case 2. copy old node
				Node *nodeOld = node;

				node = copyNode__(p, tier, nodeOld);

				deallocate_(nodeOld->tier, nodeOld);
			}

			if constexpr(Log){
				debugInfo_(node, "allocate");
			}

			return true;
		}

		bool allocateChain(NodePtr &node, size_t needed) noexcept{
			assert(!node && "allocateChain() needs new/empty node");

			auto &allocator = slab_back_();

			auto const block_size = allocator.BLOCK_SIZE - NODE_SIZE;

			auto const chunks     = needed / block_size;
			auto const remainder  = needed % block_size;

			if (chunks == 0)
				return reallocate(node, needed);

			if (auto const chunksRem = chunks + (remainder ? 1 : 0); !allocator.getFreeBlocks(chunksRem))
				return false;

			auto const tier = slab_size__ - 1;

			Node *lnode = nullptr;

			uint16_t j = 0;

			if (remainder){
				bool const b = reallocate<0>(lnode, remainder,  j++); // guaranteed not nullptr

				assert(b);

				debugInfo_(lnode, "allocate chain");
			}

			for(size_t i = 0; i < chunks; ++i){
				void *p = MyAllocator::allocate(allocator); // guaranteed not nullptr

				assert(p);

				lnode = createNode__(p, tier, lnode, j++);

				debugInfo_(lnode, "allocate chain");
			}

			node = lnode;

			return true;
		}

		void deallocate(NodePtr &node) noexcept{
			if (!node)
				return;

			if (node->next)
				return deallocateChain_(node);
			else
				return deallocateSingle_(node);

		}

		bool allocateBulk(NodePtr &node, size_t size) noexcept{
			return allocateChain(node, size);
		}

		template<typename DataSource>
		[[nodiscard]]
		bool dataCopy(NodePtr &node, DataSource const &dataSource) noexcept{
			return memory_manager_impl_::dataCopy(*this, node, dataSource);
		}

	public:
		template<Logger::Level Level>
		void log()		const noexcept;
		void printShort()	const noexcept;
		void print()		const noexcept;

	private:
		constexpr void debugInfo_(const Node *node, std::string_view message) noexcept{
			if constexpr(DEBUG_ALLOCATIONS){
				logger_fmt<DEBUG_ALLOCATIONS_LEVEL>("{:16} [tag#{:5}] {} {} {:12} {}",
						message			,
						node->tag		,
						(void *) node		,
						(void *) node->data	,
						node->capacity		,
						node->tier
				);
			}
		}

		void deallocateSingle_(NodePtr &node) noexcept{
			assert(!node->next);

			Node *it = node;

			debugInfo_(it, "deallocate");

			deallocate_(it->tier, it);

			node = nullptr;
		}

		void deallocateChain_(NodePtr &node) noexcept{
			assert(node->next);

			for(Node *it = node; it; ){
				debugInfo_(it, "deallocate chain");

				void *p = it;
				auto const tier = it->tier;

				it = it->next;

				deallocate_(tier, p);
			}

			node = nullptr;
		}

	private:
		static Node *createNode__(void *p, uint8_t tier, Node *next = nullptr, uint16_t tag = 0){
			using memory_manager_impl_::SIZES;

			Node *node;
			node		= static_cast<Node *>(p);
			node->next	= next;
			node->size	= 0;
			node->capacity	= SIZES[tier] - NODE_SIZE;
			node->head	= 0;
			node->tag	= tag;
			node->tier	= tier;

			return node;
		}

		static Node *createNode__(void *p, uint8_t tier, uint16_t tag){
			return createNode__(p, tier, nullptr, tag);
		}

		static Node *copyNode__(void *p, uint8_t tier, Node *nodeOld){
			using memory_manager_impl_::SIZES;

			memcpy(p, nodeOld, nodeOld->size + NODE_SIZE);

			Node *node;
			node		= static_cast<Node *>(p);
			node->capacity	= SIZES[tier] - NODE_SIZE;
			node->tier	= tier;

			return node;
		}

		constexpr static uint8_t findTier__(size_t needed) noexcept{
			using memory_manager_impl_::SIZES;

			for (uint8_t i = 0; i < SIZES.size(); ++i)
				if (needed <= SIZES[i])
					return i;

			return SIZES.size();
		}

		void *allocate_(uint8_t tier){
			assert(tier < slab_size__);

			switch(tier){
			case 0: return MyAllocator::allocate(slab_0_);
			case 1: return MyAllocator::allocate(slab_1_);
			case 2: return MyAllocator::allocate(slab_2_);
			case 3: return MyAllocator::allocate(slab_3_);
			}

			__builtin_unreachable();
		}

		void deallocate_(uint8_t tier, void *p) noexcept{
			assert(tier < slab_size__);

			switch(tier){
			case 0: return MyAllocator::deallocate(slab_0_, p);
			case 1: return MyAllocator::deallocate(slab_1_, p);
			case 2: return MyAllocator::deallocate(slab_2_, p);
			case 3: return MyAllocator::deallocate(slab_3_, p);
			}

			__builtin_unreachable();
		}

	private:
		template<uint8_t tier>
		constexpr static size_t cSize__(Counts const &counts, uint32_t maxClients) noexcept{
			using namespace memory_manager_impl_;

			return SIZES[tier] * calcMinimumCount<tier>(counts[tier], maxClients);
		}

		template<uint8_t tier>
		static void *cPtr__(Counts const &counts, uint32_t maxClients, MemoryResource &buffer) noexcept{
			char *base = static_cast<char *>(buffer.data());

			size_t off = 0;

			if constexpr(tier > 0) off += cSize__<0>(counts, maxClients);
			if constexpr(tier > 1) off += cSize__<1>(counts, maxClients);
			if constexpr(tier > 2) off += cSize__<2>(counts, maxClients);
			if constexpr(tier > 3) off += cSize__<3>(counts, maxClients);

			char *p = base + off;

			using namespace memory_manager_impl_;

			if constexpr(tier < SIZES.size() - 1){
				assert(off + cSize__<tier>(counts, maxClients) <= buffer.size());
			}else{
				assert(off + cSize__<tier>(counts, maxClients) == buffer.size());
			}

			return p;
		}

	private:
		MemoryResource res_;

		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[0], true> slab_0_;
		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[1], true> slab_1_;
		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[2], true> slab_2_;
		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[3], true> slab_3_;

	private:
		constexpr auto &slab_back_() noexcept{
			return slab_3_;
		}

		constexpr static uint8_t slab_size__ = 4;
	};



	template<typename MemoryResource>
	template<Logger::Level Level>
	void TieredMemoryManager<MemoryResource>::log() const noexcept{
		auto p = [](uint8_t tier, auto const &slab){
			using memory_manager_impl_::SIZES;

			logger_fmt<Level>("{:1} {:6} {:10} {:12} {:12}",
					tier			,
					slab.getNumBlocks()	,
					SIZES[tier]		,
					slab.getUsedBlocks()	,
					slab.getFreeBlocks()

			);
		};

		logger<Level>() << "IO Buffers dump";

		logger_fmt<Level>("{:1} {:6} {:10} {:12} {:12}",
					"#"			,
					"Blocks"		,
					"Block Size"		,
					"Used Blocks"		,
					"Free Blocks"
		);

		p(0, slab_0_);
		p(1, slab_1_);
		p(2, slab_2_);
		p(3, slab_3_);
	}

	template<typename MemoryResource>
	void TieredMemoryManager<MemoryResource>::printShort() const noexcept{
		auto p = [](uint8_t tier, auto const &slab){
			using memory_manager_impl_::SIZES;

			printf("%1u %6zu %9u %12zu %12zu\n",
					tier			,
					slab.getNumBlocks()	,
					SIZES[tier]		,
					slab.getUsedBlocks()	,
					slab.getFreeBlocks()

			);
		};

		p(0, slab_0_);
		p(1, slab_1_);
		p(2, slab_2_);
		p(3, slab_3_);
	}

	template<typename MemoryResource>
	void TieredMemoryManager<MemoryResource>::print() const noexcept{
		auto p = [](uint8_t tier, auto const &slab){
			using memory_manager_impl_::SIZES;

			auto const &buffer = slab.getBuffer_();

			const char *begin  = static_cast<const char *>(buffer.data());

			printf("%1u %6zu %9u %12zu %12zu %12zu KB %20p %20p\n",
					tier			,
					slab.getNumBlocks()	,
					SIZES[tier]		,
					slab.getUsedBlocks()	,
					slab.getFreeBlocks()	,
					buffer.size() / 1024	,
					(void *) (begin)	,
					(void *) (begin + buffer.size())
			);

			assert(slab.getNumBlocks() * SIZES[tier] == buffer.size());
		};

		p(0, slab_0_);
		p(1, slab_1_);
		p(2, slab_2_);
		p(3, slab_3_);

		{
			auto const &buffer = res_;
			const char *begin  = static_cast<const char *>(buffer.data());

			printf("%1s %6s %9s %12s %12s %12zu KB %20p %20p\n",
					""			,
					""			,
					""			,
					""			,
					""			,
					buffer.size() / 1024	,
					(void *) (begin)	,
					(void *) (begin + buffer.size())
			);
		}
	}

} // namespace memory_manager

#endif


#ifndef TIERED_SLAB_ALLOCATOR_H_
#define TIERED_SLAB_ALLOCATOR_H_

#include <array>
#include <algorithm>	// max

#include <cstring>
#include <cassert>

#include "slaballocator.h"

#include "memorymanager_simplebulkloader.h"
#include "memorymanager_bulkload.h"

#include "logger.h"

namespace memory_manager{

	namespace memory_manager_impl_{

		constexpr uint32_t MIN_COUNT = 2;

		constexpr uint32_t KB = 1024;

		constexpr size_t SIZES_MAX = 4;

		constexpr std::array<uint32_t, SIZES_MAX> SIZES = {
			  16 * KB,
			  64 * KB,
			 256 * KB,
			1024 * KB
		};

		constexpr std::array<uint32_t, SIZES_MAX - 1> DIVISORS = {
			 1,
			 4,
			16
		};

		template<uint8_t tier>
		constexpr uint32_t calcMinimumCount(uint32_t value, uint32_t maxClients___) noexcept{
			constexpr uint32_t MAX = std::numeric_limits<uint32_t>::max() - 1;

			auto const maxClients = std::min(maxClients___ + 1, MAX);

			if (value == std::numeric_limits<uint32_t>::max()){
				// undocumented, we **REALLY** know what we do :)
				return 0;
			}else
			if (value){
				// we know what we do
				return std::min(value, maxClients);
			}else{
				if constexpr(tier < DIVISORS.size()){
					// automatic
					return std::max(
						maxClients / DIVISORS[tier],
						MIN_COUNT
					);
				}else{
					// automatic, see max_packet_size from db_net_main.cc
					return 512;
				}
			}
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
			Node		*next		; // 8
			size_type	size		; // 4
			size_type	capacity	; // 4
			size_type	head		; // 4
			uint8_t		tier		; // 1
			char		padding[3]	; // 3

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
		bool reallocate(NodePtr &node, size_t needed) noexcept{
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
				node = createNode__(p, tier);
			}else{
				// case 2. copy old node
				Node *nodeOld = node;

				node = copyNode__(p, tier, nodeOld);

				deallocate_(nodeOld->tier, nodeOld);
			}

			debugInfo_(node, "allocate");

			return true;
		}

		bool allocateChain(NodePtr &node, size_t needed) noexcept{
			assert(!node && "allocateChain() needs new/empty node");

			auto &allocator = slab_back_();
			auto const tier = slab_size__ - 1;

			auto const block_size = allocator.BLOCK_SIZE - NODE_SIZE;
			auto const chunks = (needed + block_size - 1) / block_size;

			if (!allocator.getFreeBlocks(chunks))
				return false;

			Node *lnode = nullptr;

			for(size_t i = 0; i < chunks; ++i){
				void *p = MyAllocator::allocate(allocator); // guaranteed not nullptr

				assert(p);

				lnode = createNode__(p, tier, lnode);

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

		template<typename DataLoader>
		[[nodiscard]]
		bool bulkLoad(NodePtr &node, DataLoader const &dl) noexcept{
			return memory_manager_impl_::bulkload(*this, node, dl);
		}

	public:
		template<Logger::Level Level>
		void log()		const noexcept;
		void printShort()	const noexcept;
		void print()		const noexcept;

	private:
		constexpr void debugInfo_(const Node *node, std::string_view message) noexcept{
			if constexpr(DEBUG_ALLOCATIONS){
				logger_fmt<DEBUG_ALLOCATIONS_LEVEL>("{:16} {} {} {:12} {}",
						message			,
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
			//	auto const tier = slab_size__ - 1;

				it = it->next;

				MyAllocator::deallocate(slab_back_(), p);
			}

			node = nullptr;
		}

	private:
		template<typename MemoryManager, typename DataLoader>
		friend bool memory_manager_impl_::bulkload(MemoryManager &mm, typename MemoryManager::NodePtr &node, DataLoader const &dl) noexcept;

    		using BulkLoader = SimpleBulkLoader<NodePtr>;

		bool allocateBulk_(NodePtr &node, size_t size) noexcept{
			return allocateChain(node, size);
		}

	private:
		static Node *createNode__(void *p, uint8_t tier, Node *next = nullptr){
			using memory_manager_impl_::SIZES;

			Node *node;
			node		= static_cast<Node *>(p);
			node->next	= next;
			node->size	= 0;
			node->capacity	= SIZES[tier] - NODE_SIZE;
			node->head	= 0;
			node->tier	= tier;

			return node;
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

			if constexpr(tier < SIZES_MAX - 1){
				assert(off + cSize__<tier>(counts, maxClients) <= buffer.size());
			}else{
				assert(off + cSize__<tier>(counts, maxClients) == buffer.size());
			}

			return p;
		}

	private:
		MemoryResource res_;

		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[0]> slab_0_;
		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[1]> slab_1_;
		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[2]> slab_2_;
		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[3]> slab_3_;

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
					buffer.size()		,
					(void *) (begin)	,
					(void *) (begin + buffer.size())
			);
		}
	}

} // namespace memory_manager

#endif


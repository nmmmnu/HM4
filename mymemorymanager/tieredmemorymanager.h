#ifndef TIERED_SLAB_ALLOCATOR_H_
#define TIERED_SLAB_ALLOCATOR_H_

#include <array>
#include <algorithm>	// max

#include <cstring>
#include <cassert>

#include "slaballocator.h"

#include "logger.h"

namespace memory_manager{

	namespace memory_manager_impl_{

		constexpr static uint32_t MIN_COUNT = 2;

		constexpr static uint32_t KB = 1024;
		constexpr static uint32_t MB = 1024 * 1024;

		constexpr std::array<uint32_t, 8> SIZES = {
			 16 * KB,
			 64 * KB,
			256 * KB,
			  1 * MB,
			  4 * MB,
			 16 * MB,
			 64 * MB,
			256 * MB + 64 * KB
		};

		constexpr static uint8_t capacity2tier(uint32_t capacity) noexcept{
			switch(capacity){
			case SIZES[0] : return 0;
			case SIZES[1] : return 1;
			case SIZES[2] : return 2;
			case SIZES[3] : return 3;
			case SIZES[4] : return 4;
			case SIZES[5] : return 5;
			case SIZES[6] : return 6;
			case SIZES[7] : return 7;
			default:
				return SIZES.size();
			}
		}

		template<uint8_t tier>
		constexpr uint32_t calcMinimumCount(uint32_t value, uint32_t maxClients___) noexcept{

			auto calcMaxClients = [](uint32_t maxClients){
				auto const MAX = std::numeric_limits<uint32_t>::max() - 1;

				return maxClients >= MAX ? MAX : maxClients + 1;
			};

			auto const maxClients = calcMaxClients(maxClients___);

			constexpr uint32_t DIVISORS[] = {
				0b0'1			,	//     1
				0b0'100			,	//     4
				0b0'1000'0		,	//    16
				0b0'1000'000		,	//    64
				0b0'1000'0000'0		,	//   256
				0b0'1000'0000'000	,	//  1024
				0b0'1000'0000'0000'0	,	//  4096
				0b0'1000'0000'0000'000		// 16384
			};

			if (value == std::numeric_limits<uint32_t>::max()){
				// undocumented, we **REALLY** know what we do :)
				return 0;
			}else
			if (value){
				// we know what we do
				return std::min(value, maxClients);
			}else{
				// automatic
				return std::max(
					maxClients / DIVISORS[tier],
					MIN_COUNT
				);
			}
		}

	} // namespace memory_manager_impl_



	template<typename MemoryResource>
	class TieredMemoryManager{
	//	using MemoryResource	= MyBuffer::MMapMemoryResource;

		constexpr static auto SIZE = memory_manager_impl_::SIZES.size();

	public:
		using size_type	= uint32_t;
		using Counts	= std::array<size_type, SIZE>;

		struct Context{
			using size_type	= uint32_t;

			char		*data		= nullptr;
			size_type	size		= 0;
			size_type	capacity	= 0;

		public:
			constexpr void clear() noexcept{
				data		= nullptr;
				size		= 0;
				capacity	= 0;
			}

			constexpr void swap(Context &other) noexcept{
				using std::swap;

				swap(data,	other.data	);
				swap(size,	other.size	);
				swap(capacity,	other.capacity	);
			}
		};

	public:
		TieredMemoryManager(Counts const &counts, uint32_t maxClients = 0) :
			res_(
				cSize__<0>(counts, maxClients) +
				cSize__<1>(counts, maxClients) +
				cSize__<2>(counts, maxClients) +
				cSize__<3>(counts, maxClients) +
				cSize__<4>(counts, maxClients) +
				cSize__<5>(counts, maxClients) +
				cSize__<6>(counts, maxClients) +
				cSize__<7>(counts, maxClients)
			),

			slab_0_( cPtr__<0>(counts, maxClients, res_), cSize__<0>(counts, maxClients), MyBuffer::from_bytes{} ),
			slab_1_( cPtr__<1>(counts, maxClients, res_), cSize__<1>(counts, maxClients), MyBuffer::from_bytes{} ),
			slab_2_( cPtr__<2>(counts, maxClients, res_), cSize__<2>(counts, maxClients), MyBuffer::from_bytes{} ),
			slab_3_( cPtr__<3>(counts, maxClients, res_), cSize__<3>(counts, maxClients), MyBuffer::from_bytes{} ),
			slab_4_( cPtr__<4>(counts, maxClients, res_), cSize__<4>(counts, maxClients), MyBuffer::from_bytes{} ),
			slab_5_( cPtr__<5>(counts, maxClients, res_), cSize__<5>(counts, maxClients), MyBuffer::from_bytes{} ),
			slab_6_( cPtr__<6>(counts, maxClients, res_), cSize__<6>(counts, maxClients), MyBuffer::from_bytes{} ),
			slab_7_( cPtr__<7>(counts, maxClients, res_), cSize__<7>(counts, maxClients), MyBuffer::from_bytes{} ){}

		TieredMemoryManager(uint32_t maxClients = 0) :
					TieredMemoryManager( { /* zeroes */ }, maxClients){}

		constexpr static std::string_view getName(){
			return "TieredMemoryManager";
		}

	public:
		bool allocate(Context &context, size_t needed){
			using memory_manager_impl_::SIZES;

			auto tier = findTier_(needed);

			auto const context_tier = capacity2tier__(context);

			if (context.data && context_tier >= tier)
				return true;

			if (tier >= SIZES.size())
				return false;

			void *p = nullptr;

			for(; tier < SIZES.size(); ++tier)
				if (p = allocate_(tier); p)
					break;

			if (!p)
				return false;

			if (context.data && context.size)
				memcpy(p, context.data, context.size);

			if (context.data)
				deallocate_(context_tier, context.data);

			context.data     = static_cast<char *>(p);
			context.capacity = SIZES[tier];

			return true;
		}

		void deallocate(Context &context) noexcept{
			if (context.data){
				auto const context_tier = capacity2tier__(context);

				deallocate_(context_tier, context.data);
			}

			context.clear();
		}

	public:
		template<Logger::Level Level>
		void log() const noexcept{
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
			p(4, slab_4_);
			p(5, slab_5_);
			p(6, slab_6_);
			p(7, slab_7_);
		}

		void printShort() const noexcept{
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
			p(4, slab_4_);
			p(5, slab_5_);
			p(6, slab_6_);
			p(7, slab_7_);
		}

		void print() const noexcept{
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
			p(4, slab_4_);
			p(5, slab_5_);
			p(6, slab_6_);
			p(7, slab_7_);

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

	private:
		constexpr static uint8_t findTier_(size_t needed) noexcept{
			using memory_manager_impl_::SIZES;

			for (uint8_t i = 0; i < SIZES.size(); ++i)
				if (needed <= SIZES[i])
					return i;

			return SIZES.size();
		}

		void *allocate_(uint8_t tier){
			using memory_manager_impl_::SIZES;

			assert(tier < SIZES.size());

			switch(tier){
			case 0: return MyAllocator::allocate(slab_0_);
			case 1: return MyAllocator::allocate(slab_1_);
			case 2: return MyAllocator::allocate(slab_2_);
			case 3: return MyAllocator::allocate(slab_3_);
			case 4: return MyAllocator::allocate(slab_4_);
			case 5: return MyAllocator::allocate(slab_5_);
			case 6: return MyAllocator::allocate(slab_6_);
			case 7: return MyAllocator::allocate(slab_7_);
			}

			__builtin_unreachable();
		}

		void deallocate_(uint8_t tier, void *p) noexcept{
			using memory_manager_impl_::SIZES;

			assert(tier < SIZES.size());

			switch(tier){
			case 0: return MyAllocator::deallocate(slab_0_, p);
			case 1: return MyAllocator::deallocate(slab_1_, p);
			case 2: return MyAllocator::deallocate(slab_2_, p);
			case 3: return MyAllocator::deallocate(slab_3_, p);
			case 4: return MyAllocator::deallocate(slab_4_, p);
			case 5: return MyAllocator::deallocate(slab_5_, p);
			case 6: return MyAllocator::deallocate(slab_6_, p);
			case 7: return MyAllocator::deallocate(slab_7_, p);
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
			if constexpr(tier > 4) off += cSize__<4>(counts, maxClients);
			if constexpr(tier > 5) off += cSize__<5>(counts, maxClients);
			if constexpr(tier > 6) off += cSize__<6>(counts, maxClients);
			if constexpr(tier > 7) off += cSize__<7>(counts, maxClients);

			char *p = base + off;

			if constexpr(tier < 7){
				assert(off + cSize__<tier>(counts, maxClients) <=  buffer.size());
			}else{
				assert(off + cSize__<tier>(counts, maxClients) == buffer.size());
			}

			return p;
		}

		constexpr static uint8_t capacity2tier__(Context const &context) noexcept{
			using namespace memory_manager_impl_;

			return capacity2tier(context.capacity);
		}

	private:
		MemoryResource res_;

		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[0]> slab_0_;
		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[1]> slab_1_;
		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[2]> slab_2_;
		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[3]> slab_3_;
		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[4]> slab_4_;
		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[5]> slab_5_;
		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[6]> slab_6_;
		MyAllocator::SlabAllocator<memory_manager_impl_::SIZES[7]> slab_7_;
	};



	template<typename MemoryResource>
	constexpr void swap(typename TieredMemoryManager<MemoryResource>::Context &a, typename TieredMemoryManager<MemoryResource>::Context &b){
		a.swap(b);
	}

} // namespace memory_manager

#endif


#ifndef CUCKOO_FILTER_H_
#define CUCKOO_FILTER_H_

#include <array>
#include <string_view>
#include <cstdint>
#include <cstring>
#include <cassert>

#include "murmur_hash_64a.h"
#include "murmur_hash_mixer.h"
#include "myendian.h"

namespace cuckoo_filter{

	namespace cuckoo_filter_impl_{
		[[nodiscard]]
		auto hash(std::string_view s){
			uint64_t const seed = 0;

			return murmur_hash64a(s.data(), s.size(), seed);
		}

		template<typename FPT>
		[[nodiscard]]
		auto hashAltIndex(size_t const index, FPT const fp, size_t const size){
			uint64_t const seed = 0;

			uint64_t const hash = murmur_hash64a(&fp, sizeof(FPT), seed);

			return (index ^ hash) % size;
		}

		template <typename T>
		constexpr void swapHostToBE(T &a, T &b){
			static_assert(
				std::is_same_v<T, uint8_t > ||
				std::is_same_v<T, uint16_t> ||
				std::is_same_v<T, uint32_t> ||
				std::is_same_v<T, uint64_t>
			);

			T const tmp = a;
			a = betoh(b);
			b = htobe(tmp);
		}

		// ============================

		template<typename FPT, uint8_t Slots>
		class CuckooFilterBase{
			static_assert(
				std::is_same_v<FPT, uint8_t > ||
				std::is_same_v<FPT, uint16_t> ||
				std::is_same_v<FPT, uint32_t>
			);

			static_assert(
				Slots == 1 ||
				Slots == 2 ||
				Slots == 4 ||
				Slots == 8
			);

		private:
			constexpr static size_t MaxKicks	= 256;

			struct RollBack{
				size_t	index;
				FPT	fp;
				uint8_t	slot;
			};

			// 4KB, keep it small, because is on the stack.
			using RollBackArray = std::array<RollBack, MaxKicks>;

		private:
			size_t size_;

		public:
			constexpr CuckooFilterBase(size_t size) : size_(size){
				assert(size);
			}

			constexpr size_t size() const{
				return size_;
			}

			constexpr size_t bytes() const{
				return bytes(size_);
			}

			constexpr static size_t bytes(size_t size){
				return sizeof(FPT) * Slots * size;
			}

		public:
			void clear(FPT *table) const{
				memset(table, 0, bytes());
			}

			void load(FPT *table, const void *src) const{
				memcpy(table, src, bytes());
			}

			void store(const FPT *table, void *dest) const{
				memcpy(dest, table, bytes());
			}

		public:
			[[nodiscard]]
			bool insert(FPT *table, std::string_view s){
				auto const hash = cuckoo_filter_impl_::hash(s);

				auto const fpo  = this->createFP__(hash);

				auto const index1 = hash % size_;

				if (tryInsert__<1>(fpo, ix(table, index1)))
					return true;

				auto const index2 = hashAltIndex(index1, fpo, size_);

				if (tryInsert__<1>(fpo, ix(table, index2)))
					return true;

				RollBackArray rollback;

				auto fp    = fpo;
				auto index = index1;

				for (size_t i = 0; i < MaxKicks; ++i){
					auto const slot = rand__(fp, i);

					swapHostToBE(fp, *ix(table, index, slot));

					rollback[i] = { index, fp, slot };

					index = hashAltIndex(index, fp, size_);

					if (tryInsert__(fp, ix(table, index)))
						return true;
				}

				// rollback

				for (auto it = std::rbegin(rollback); it != std::rend(rollback); ++it)
					*ix(table, it->index, it->slot) = htobe(it->fp);

				return false;
			}

			bool remove(FPT *table, std::string_view s){
				auto const hash = cuckoo_filter_impl_::hash(s);
				auto const fp   = createFP__(hash);

				auto const index1 = hash % size_;

				if (auto *p = tryFind__(fp, ix(table, index1)); p){
					*p = 0;
					return true;
				}

				auto const index2 = hashAltIndex(index1, fp, size_);

				if (auto *p = tryFind__(fp, ix(table, index2)); p){
					*p = 0;
					return true;
				}

				return false;
			}

			[[nodiscard]]
			bool lookup(const FPT *table, std::string_view s) const{
				auto const hash = cuckoo_filter_impl_::hash(s);
				auto const fp   = createFP__(hash);

				auto const index1 = hash % size_;

				if (const auto *p = tryFind__(fp, ix(table, index1)); p)
					return true;

				auto const index2 = hashAltIndex(index1, fp, size_);

				if (const auto *p = tryFind__(fp, ix(table, index2)); p)
					return true;

				return false;

			}

		private:
			constexpr static auto *ix(FPT *table, size_t index, size_t slot = 0){
				return table + index * Slots + slot;
			}

			constexpr static const auto *ix(const FPT *table, size_t index, size_t slot = 0){
				return table + index * Slots + slot;
			}

		private:
			[[nodiscard]]
			constexpr static uint8_t rand2__(uint64_t fp, size_t iteration){
				constexpr uint8_t SlotsMask = Slots - 1;

				uint64_t const seed = fp ^ iteration;

				uint64_t const mixed = murmur_hash_mixer64_nz(seed);

				return mixed & SlotsMask;
			}

			[[nodiscard]]
			constexpr static uint8_t rand__(uint64_t fp, size_t iteration){
				if constexpr(Slots == 1){
					return 0;
				}else{
					return rand2__(fp, iteration);
				}
			}

		private:
			template<bool Initial = false>
			[[nodiscard]]
			static bool tryInsert__(FPT fp, FPT *bucket){
				auto const fp_be = htobe(fp);

				for (auto it = bucket; it < bucket + Slots; ++it){
					if constexpr(Initial){
						if (*it == fp_be)
							return true;
					}

					if (*it == 0){
						*it = fp_be;
						return true;
					}
				}

				return false;
			}

			[[nodiscard]]
			constexpr static const FPT *tryFind__(FPT fp, const FPT *bucket){
				auto const fp_be = htobe(fp);

				for (auto it = bucket; it < bucket + Slots; ++it)
					if (*it == fp_be)
						return it;

				return nullptr;
			}

			[[nodiscard]]
			constexpr static FPT *tryFind__(FPT fp, FPT *bucket){
				auto const fp_be = htobe(fp);

				for (auto it = bucket; it < bucket + Slots; ++it)
					if (*it == fp_be)
						return it;

				return nullptr;
			}

		private:
			[[nodiscard]]
			constexpr static FPT createFP__(uint64_t hash){
				auto const fp = static_cast<FPT>(hash);

				return fp ? fp : fp | 0x1;
			}
		};

	} // namespace cuckoo_filter_impl_

	template<typename FPT, uint8_t Slots = 4>
	using CuckooFilter	= cuckoo_filter_impl_::CuckooFilterBase<FPT, Slots>;

	using CuckooFilter8	= CuckooFilter<uint8_t	>;
	using CuckooFilter16	= CuckooFilter<uint16_t	>;
	using CuckooFilter32	= CuckooFilter<uint32_t	>;


} // namespace cuckoo_filter

#endif


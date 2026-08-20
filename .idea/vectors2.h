
#if 0
	namespace simhash_impl_{

		namespace RND{
#if 0
			struct Container{
				constexpr Container(){
					MurmurHashMixer64 gen;

					for(auto &x : data)
						x = MD::distribution<int8_t>(gen());
				}

				constexpr auto get(){
					constexpr size_t mask = MAX - 1;

					return data[id++ & mask];
				}

				constexpr auto operator()(){
					return get();
				}

			private:
				constexpr static size_t MAX = 16 * 1024 * 1024;

				int8_t data[MAX]{};
				size_t id = 0;
			};

			auto &M = *new Container();
#endif

			struct Xoshiro128PlusPlus{
				constexpr Xoshiro128PlusPlus(uint32_t seed = 0) noexcept{
					uint32_t z = seed;
					for (auto &val : s_){
						z += 0x9E3779B9u;

						uint32_t t = z;

						t ^= t >> 16;
						t *= 0x21f0aaadu;
						t ^= t >> 15;
						t *= 0x735a2d97u;
						t ^= t >> 15;

						val = t;
					}
				}

				constexpr uint32_t operator()() noexcept{
					uint32_t const result = rotl_(s_[0] + s_[3], 7) + s_[0];

					uint32_t const t = s_[1] << 9;

					s_[2] ^= s_[0];
					s_[3] ^= s_[1];
					s_[1] ^= s_[2];
					s_[0] ^= s_[3];

					s_[2] ^= t;
					s_[3] = rotl_(s_[3], 11);

					return result;
				}

			private:
				constexpr static inline uint32_t rotl_(uint32_t x, uint8_t k) noexcept{
					#if __cplusplus >= 202002L
						return std::rotl(x, k);
					#else
						return (x << k) | (x >> (32 - k));
					#endif
				}

			private:
				uint32_t s_[4]{};
			};


		} // namespace RND

		namespace Accumulator{

			using type = int32_t;

			constexpr auto M8 = std::numeric_limits<int8_t	>::max();
			constexpr auto MT = std::numeric_limits<type	>::max();

			constexpr auto MaxDimensions = 100'000u;

			static_assert(M8 * M8 * MaxDimensions < MT);

		} // namespace Accumulator

		template<typename Generator>
		constexpr bool isAboveHyperplane(CTVector<int8_t> const a, Generator &generator){
			Accumulator::type dot = 0;

			for (uint32_t i = 0; i < a.size(); ++i)
				dot += a[i] * MD::distribution<int8_t>(generator());

			return dot > 0;
		}

		/*
		template<typename Generator>
		bool isAboveHyperplaneBit(const BitVector &a, Generator &generator) {
			Accumulator::type dot = 0;

			FORCE_VECTORIZE
			for (uint32_t i = 0; i < a.size(); ++i){
				auto const hyperplane = MD::distribution<int8>(generator());
				dot += a[i] ? +hyperplane : -hyperplane;
			}

			return dot > 0;
		}
		*/

		template<typename T, typename Generator>
		constexpr T simhash(CTVector<int8_t> const a, Generator &generator){
			static_assert(
				std::is_same_v<T, uint8_t > ||
				std::is_same_v<T, uint16_t> ||
				std::is_same_v<T, uint32_t> ||
				std::is_same_v<T, uint64_t>
			);

			size_t const bytes = sizeof(T) * 8;

			T x = 0;

			for(size_t i = 0; i < bytes; ++i){
				x <<= 1;
				x |=  isAboveHyperplane(a, generator) ? T{ 1 } : T{ 0 };
			}

			return x;
		}

	} // namespace simhash_impl_

	template<typename T, typename F>
	void simhashBands___(CTVector<int8_t> const a, uint8_t bands, F f, uint64_t seed = 0){
		using namespace simhash_impl_;

	//	MurmurHashMixer64 generator{ seed };
		RND::Xoshiro128PlusPlus generator{ (uint32_t) seed };

		for(uint8_t id = 0; id < bands; ++id)
			f(id, simhash<T>(a, generator));
	}

#endif


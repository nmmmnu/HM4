#ifndef HYPER_LOG_LOG_H_
#define HYPER_LOG_LOG_H_

#include <cstdint>
#include <cassert>
#include <cstring>
#include <string_view>
#include <algorithm>	// std::clamp
#include <type_traits>

#include <cmath>

namespace hyperloglog{

	namespace hyperloglog_implementation{
		struct CalcAddResult{
			uint32_t index;
			uint8_t  rank;
		};

		CalcAddResult calcAdd(const char *s, size_t size, uint8_t bits);
		double estimate(const uint8_t *data, uint32_t size);
	}



	struct HyperLogLogOperationsRaw;



	struct HyperLogLogRaw{
		uint8_t  bits;
		uint32_t m = 1 << bits;

		constexpr HyperLogLogRaw(uint8_t bits) : bits(bits){
			assert(bits > 4 && bits < 30);
		}

		// --------------------------

		constexpr double error(){
			return 1.04 / sqrt(m);
		}

		// --------------------------

		void clear(uint8_t *M) const{
			memset(M, 0, m);
		}

		void load(uint8_t *M, const void *src) const{
			memcpy(M, src, m);
		}

		void store(const uint8_t *M, void *dest) const{
			memcpy(dest, M, m);
		}

		// --------------------------

		[[nodiscard]]
		bool add(uint8_t *M, const char *s, size_t size) const{
			auto const &[index, rank] = hyperloglog_implementation::calcAdd(s, size, bits);

			if (M[index] >= rank)
				return false;

			M[index] = rank;

			return true;
		}

		[[nodiscard]]
		bool add(uint8_t *M, std::string_view s) const{
			return add(M, s.data(), s.size());
		}

		[[nodiscard]]
		bool add(uint8_t *M, const void *s, size_t size) const{
			return add(M, static_cast<const char *>(s), size);
		}

		// --------------------------

		constexpr static double estimate(){
			return 0;
		}

		double estimate(const uint8_t *M) const{
			return hyperloglog_implementation::estimate(M, m);
		}

		void merge(uint8_t *M, const uint8_t *OM) const{
			for(size_t i = 0; i < m; ++i)
				M[i] = std::max(M[i], OM[i]);
		}

		// --------------------------

		constexpr HyperLogLogOperationsRaw getOperations() const;
	};



	struct HyperLogLogOperationsRaw{
		uint8_t bits;

	public:
		constexpr HyperLogLogOperationsRaw(uint8_t bits) : bits(bits){
			assert(bits > 4 && bits < 30);
		}

	private:
		// copy paste in order to save parameter types.

		constexpr static double e(uint8_t *){
			return 0;
		}

		template<typename ...Args>
		double e(uint8_t *_, const uint8_t *first, const Args *...rest) const{
			static_assert((std::is_same_v<Args, uint8_t> && ...), "All elements to estimate must be 'const uint8_t *'");

			HyperLogLogRaw hll{bits};

			hll.load(_, first);

			((hll.merge(_, rest)), ...);

			return hll.estimate(_);
		}

	public:
		constexpr static double intersect(uint8_t *){
			return 0;
		}

		double intersect(uint8_t *_, const uint8_t *A) const{
			return e(_, A);
		}

		double intersect(uint8_t *_, const uint8_t *A, const uint8_t *B) const{
		//	^ = intersect, u = union

		//	A ^ B =
		//		+ (A) + (B)
		//		- (A u B)

		//	2 + 1 = 3 calcs total

			return
				+ e(_, A) + e(_, B)
				- e(_, A, B)
			;
		}

		double intersect(uint8_t *_, const uint8_t *A, const uint8_t *B, const uint8_t *C) const{
		//	^ = intersect, u = union

		//	A ^ B ^ C =
		//		+ (A) + (B) + (C)
		//		- (A u B) - (B u C) - (C u A)
		//		+ (A u B u C)

		//	3 + 3 + 1 = 7 calcs total

			return
				+ e(_, A) + e(_, B) + e(_, C)
				- e(_, A, B) - e(_, B, C) - e(_, C, A)
				+ e(_, A, B, C)
			;
		}

		double intersect(uint8_t *_, const uint8_t *A, const uint8_t *B, const uint8_t *C, const uint8_t *D) const{

		//	https://upwikibg.top/wiki/Inclusion%E2%80%93exclusion_principle
		//	https://math.stackexchange.com/questions/688019/what-is-the-inclusion-exclusion-principle-for-4-sets

		//	^ = intersect, u = union

		//	A ^ B ^ C ^ D =
		//		+ (A) + (B) + (C) + (D)
		//		- (A u B) - (A u C) - (A u D) - (B u C) - (B u D) - (C u D)
		//		+ (A u B u C) + (A u B u D) + (A u C u D) + (B u C u D)
		//		- (A u B u C u D)

		//	4 + 6 + 4 + 1 = 15 calcs total

			return
				+ e(_, A) + e(_, B) + e(_, C) + e(_, D)
				- e(_, A, B) - e(_, A, C) - e(_, A, D) - e(_, B, C) - e(_, B, D) - e(_, C, D)
				+ e(_, A, B, C) + e(_, A, B, D) + e(_, A, C, D) + e(_, B, C, D)
				- e(_, A, B, C, D)
			;
		}

		double intersect(uint8_t *_, const uint8_t *A, const uint8_t *B, const uint8_t *C, const uint8_t *D, const uint8_t *E) const{

		//	https://math.stackexchange.com/questions/3235843/what-is-the-inclusion-exclusion-principle-for-five-sets

		//	^ = intersect, u = union

		//	A ^ B ^ C ^ D ^ E =
		//		+ A + B + C + D + E
		//		- A u B - A u C - A u D - A u E - B u C - B u D - B u E - C u D - C u E - D u E
		//		+ A u B u C + A u B u D + A u B u E + A u C u D + A u C u E + A u D u E + B u C u D + B u C u E + B u D u E + C u D u E
		//		- A u B u C u D - A u B u C u E - A u B u D u E - A u C u D u E - B u C u D u E
		//		+ A u B u C u D u E

		//	5 + 10 + 10 + 5 + 1 = 31 calcs total

			return
				+ e(_, A) + e(_, B) + e(_, C) + e(_, D) + e(_, E)
				- e(_, A, B) - e(_, A, C) - e(_, A, D) - e(_, A, E) - e(_, B, C) - e(_, B, D) - e(_, B, E) - e(_, C, D) - e(_, C, E) - e(_, D, E)
				+ e(_, A, B, C) + e(_, A, B, D) + e(_, A, B, E) + e(_, A, C, D) + e(_, A, C, E) + e(_, A, D, E) + e(_, B, C, D) + e(_, B, C, E) + e(_, B, D, E) + e(_, C, D, E)
				- e(_, A, B, C, D) - e(_, A, B, C, E) - e(_, A, B, D, E) - e(_, A, C, D, E) - e(_, B, C, D, E)
				+ e(_, A, B, C, D, E)
			;
		}

	public:
		double jaccard(uint8_t *_, const uint8_t *A, const uint8_t *B) const{
		//	^ = intersect, u = union

		//	naive:
		//	J = (A ^ B) / (A u B)
		//	return ( intersect(_, A, B) ) / e(_, A, B);

		//	better with single intersect
		//	J = (A ^ B) / (A u B)
		//	J = ( (A) + (B) - (A u B) ) / (A u B)

			auto const U = e(_, A, B);

			if (std::abs(U) < 0.001)
				return 0.;

			auto const J = ( e(_, A) + e(_, B) - U ) / U;

			return std::clamp(J, 0.00, 1.00);
		}

		double overlap(uint8_t *_, const uint8_t *A, const uint8_t *B) const{
		//	^ = intersect, u = union

		//	J = (A ^ B) / min(A, B)
		//	J = ( (A) + (B) - (A u B) ) / min(A, B)

			auto const U = e(_, A, B);

			auto const a = e(_, A);
			auto const b = e(_, B);

			auto const mab = std::min(a, b);

			if (std::abs(mab) < 0.001)
				return 0.;

			auto const O = (a + b - U) / mab;

			return std::clamp(O, 0.00, 1.00);
		}
	};



	constexpr auto HyperLogLogRaw::getOperations() const -> HyperLogLogOperationsRaw{
		return HyperLogLogOperationsRaw{bits};
	}

} // namespace hyperloglog

#endif


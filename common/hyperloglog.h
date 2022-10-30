#ifndef HYPER_LOG_LOG_H_
#define HYPER_LOG_LOG_H_

#include <cstdint>
#include <cassert>
#include <cstring>
#include <string_view>

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

	template<uint8_t Bits>
	struct HyperLogLogOperations;





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

		void add(uint8_t *M, const char *s, size_t size) const{
			auto const &[index, rank] = hyperloglog_implementation::calcAdd(s, size, bits);

			M[index] = std::max(M[index], rank);
		}

		auto add(uint8_t *M, std::string_view s) const{
			return add(M, s.data(), s.size());
		}

		auto add(uint8_t *M, const void *s, size_t size) const{
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





	template<uint8_t Bits>
	struct HyperLogLog{
		constexpr static auto getRaw(){
			return HyperLogLogRaw{Bits};
		}

		// --------------------------

		constexpr static uint8_t  bits       = getRaw().bits;
		constexpr static uint32_t m          = getRaw().m;

		static_assert(bits > 4 && bits < 30);

		// --------------------------

		uint8_t M[m];

		// --------------------------

		constexpr HyperLogLog(){
			clear();
		}

		constexpr HyperLogLog(std::nullptr_t){
			// no clear.
		}

		constexpr HyperLogLog(const void *src){
			load(src);
		}

		constexpr static double error(){
			return getRaw().error();
		}

		// --------------------------

		void clear(){
			return getRaw().clear(std::data(M));
		}

		void load(const void *src){
			return getRaw().load(std::data(M), src);
		}

		void store(void *dest) const{
			return getRaw().load(std::data(M), dest);
		}

		// --------------------------

		template<typename ...Args>
		void add(Args &&...args){
			return getRaw().add(std::data(M), std::forward<Args>(args)...);
		}

		// --------------------------

		double estimate() const{
			return getRaw().estimate(std::data(M));
		}

		void merge(HyperLogLog const &other){
			return getRaw().merge(std::data(M), std::data(other.M));
		}

		// --------------------------

		operator uint8_t *(){
			return M;
		}

		operator const uint8_t *() const{
			return M;
		}

		// --------------------------

		constexpr static HyperLogLogOperations<Bits> getOperations();
	};





	struct HyperLogLogOperationsRaw{
		uint8_t bits;

	public:
		constexpr HyperLogLogOperationsRaw(uint8_t bits) : bits(bits){
			assert(bits > 4 && bits < 30);
		}

	public:
		constexpr static double merge(uint8_t *){
			return 0;
		}

		double merge(uint8_t *, const uint8_t *A) const{
			HyperLogLogRaw hll{bits};

			return hll.estimate(A);
		}

		double merge(uint8_t *_, const uint8_t *A, const uint8_t *B) const{
			HyperLogLogRaw hll{bits};

			hll.load (_, A);
			hll.merge(_, B);
			return hll.estimate(_);
		}

		double merge(uint8_t *_, const uint8_t *A, const uint8_t *B, const uint8_t *C) const{
			HyperLogLogRaw hll{bits};

			hll.load (_, A);
			hll.merge(_, B);
			hll.merge(_, C);
			return hll.estimate(_);
		}

		double merge(uint8_t *_, const uint8_t *A, const uint8_t *B, const uint8_t *C, const uint8_t *D) const{
			HyperLogLogRaw hll{bits};

			hll.load (_, A);
			hll.merge(_, B);
			hll.merge(_, C);
			hll.merge(_, D);
			return hll.estimate(_);
		}

		double merge(uint8_t *_, const uint8_t *A, const uint8_t *B, const uint8_t *C, const uint8_t *D, const uint8_t *E) const{
			HyperLogLogRaw hll{bits};

			hll.load (_, A);
			hll.merge(_, B);
			hll.merge(_, C);
			hll.merge(_, D);
			hll.merge(_, E);
			return hll.estimate(_);
		}

	private:
		template<typename ...Args>
		auto e(uint8_t *_, Args &&...args) const{
			return merge(_, std::forward<Args>(args)...);
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
	};





	template<uint8_t Bits>
	struct HyperLogLogOperations{
		using HLL = HyperLogLog<Bits>;

		constexpr static uint8_t  bits = HLL::getRaw().bits;
		constexpr static uint32_t m    = HLL::getRaw().m;

		static_assert(bits > 4 && bits < 30);

	public:
		constexpr static double merge(){
			return 0;
		}

		static double merge(const uint8_t *a){
			// if we go via merge__,
			// a buffer will be allocated...
			return HLL::getRaw().estimate(a);
		}

		static double merge(const uint8_t *a, const uint8_t *b){
			return merge__(a, b);
		}

		static double merge(const uint8_t *a, const uint8_t *b, const uint8_t *c){
			return merge__(a, b, c);
		}

		static double merge(const uint8_t *a, const uint8_t *b, const uint8_t *c, const uint8_t *d){
			return merge__(a, b, c, d);
		}

	public:
		constexpr static double intersect(){
			return merge();
		}

		static double intersect(const uint8_t *a){
			return merge(a);
		}

		static double intersect(const uint8_t *a, const uint8_t *b){
			return intersect__(a, b);
		}

		static double intersect(const uint8_t *a, const uint8_t *b, const uint8_t *c){
			return intersect__(a, b, c);
		}

		static double intersect(const uint8_t *a, const uint8_t *b, const uint8_t *c, const uint8_t *d){
			return intersect__(a, b, c, d);
		}

	private:
		template<typename F, typename ...Args>
		static auto op__(F f, Args &&...args){
			static_assert(sizeof...(args) >= 0 && sizeof...(args) <= 5);

			uint8_t tmp[m];

			auto operations = HLL::getRaw().getOperations();

			return f(operations, tmp, std::forward<Args>(args)...);
		}

		template<typename ...Args>
		static auto merge__(Args &&...args){
			auto f = [](HyperLogLogOperationsRaw &operations, uint8_t *buffer, auto &&...args){
				return operations.merge(buffer, std::forward<Args>(args)...);
			};

			return op__(f, args...);
		}

		template<typename ...Args>
		static auto intersect__(Args &&...args){
			auto f = [](HyperLogLogOperationsRaw &operations, uint8_t *buffer, auto &&...args){
				return operations.intersect(buffer, std::forward<Args>(args)...);
			};

			return op__(f, args...);
		}
	};





	constexpr auto HyperLogLogRaw::getOperations() const -> HyperLogLogOperationsRaw{
		return HyperLogLogOperationsRaw{bits};
	}

	template<uint8_t Bits>
	constexpr auto HyperLogLog<Bits>::getOperations() -> HyperLogLogOperations<Bits>{
		return HyperLogLogOperations<Bits>{};
	}

} // namespace hyperloglog

#endif


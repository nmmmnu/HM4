#ifndef HYPER_LOG_LOG_H_
#define HYPER_LOG_LOG_H_

#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <string_view>
#include <array>

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
	struct HyperLogLogConfig{
		constexpr static size_t   bits	= HyperLogLogRaw{Bits}.bits;
		constexpr static uint32_t m	= HyperLogLogRaw{Bits}.m;

		static_assert(bits > 4 && bits < 30);
	};





	template<uint8_t Bits>
	struct HyperLogLog{
		constexpr static uint8_t  bits = HyperLogLogRaw{Bits}.bits;
		constexpr static uint32_t m    = HyperLogLogRaw{Bits}.m;

		static_assert(bits > 4 && bits < 30);

		// --------------------------

		std::array<uint8_t, m> M;

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

		// --------------------------

		void clear(){
			return HyperLogLogRaw{Bits}.clear(std::data(M));
		}

		void load(const void *src){
			return HyperLogLogRaw{Bits}.load(std::data(M), src);
		}

		void store(void *dest) const{
			return HyperLogLogRaw{Bits}.load(std::data(M), dest);
		}

		// --------------------------

		template<typename ...Args>
		void add(Args &&...args){
			return HyperLogLogRaw{Bits}.add(std::data(M), args...);
		}

		// --------------------------

		double estimate() const{
			return HyperLogLogRaw{Bits}.estimate(std::data(M));
		}

		void merge(HyperLogLog const &other){
			return HyperLogLogRaw{Bits}.merge(std::data(M), std::data(other.M));
		}

		// --------------------------

		operator uint8_t *(){
			return M.data();
		}

		operator const uint8_t *() const{
			return M.data();
		}

		// --------------------------

		constexpr static HyperLogLogOperations<Bits> getOperations();
	};





	struct HyperLogLogOperationsRaw{
	private:
		HyperLogLogRaw hll;

	public:
		constexpr HyperLogLogOperationsRaw(uint8_t bits) : hll(bits){
			assert(bits > 4 && bits < 30);
		}

	public:
		constexpr static double merge(uint8_t *){
			return 0;
		}

		double merge(uint8_t *, const uint8_t *A) const{
			return hll.estimate(A);
		}

		double merge(uint8_t *_, const uint8_t *A, const uint8_t *B) const{
			hll.load (_, A);
			hll.merge(_, B);
			return hll.estimate(_);
		}

		double merge(uint8_t *_, const uint8_t *A, const uint8_t *B, const uint8_t *C) const{
			hll.load (_, A);
			hll.merge(_, B);
			hll.merge(_, C);
			return hll.estimate(_);
		}

		double merge(uint8_t *_, const uint8_t *A, const uint8_t *B, const uint8_t *C, const uint8_t *D) const{
			hll.load (_, A);
			hll.merge(_, B);
			hll.merge(_, C);
			hll.merge(_, D);
			return hll.estimate(_);
		}

	private:
		template<typename ...Args>
		auto e(uint8_t *_, Args &&...args) const{
			return merge(_, args...);
		}

	public:
		constexpr static double intersect(uint8_t *){
			return 0;
		}

		double intersect(uint8_t *, const uint8_t *A) const{
			return hll.estimate(A);
		}

		double intersect(uint8_t *_, const uint8_t *A, const uint8_t *B) const{
		//	^ = intersect, u = union

		//	A ^ B =
		//		+ (A) + (B)
		//		- (A u B)

		//	3 calcs total

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

		//	7 calcs total

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

		//	15 calcs total

			return
				+ e(_, A) + e(_, B) + e(_, C) + e(_, D)
				- e(_, A, B) - e(_, A, C) - e(_, A, D) - e(_, B, C) - e(_, B, D) - e(_, C, D)
				+ e(_, A, B, C) + e(_, A, B, D) + e(_, A, C, D) + e(_, B, C, D)
				- e(_, A, B, C, D)
			;
		}
	};





	template<uint8_t Bits>
	struct HyperLogLogOperations{
		constexpr static uint8_t  bits = HyperLogLogRaw{Bits}.bits;
		constexpr static uint32_t m    = HyperLogLogRaw{Bits}.m;

		static_assert(bits > 4 && bits < 30);

		using HLL = HyperLogLog<Bits>;

	public:
		constexpr static double merge(){
			return 0;
		}

		static double merge(const uint8_t *a){
			HyperLogLogRaw hll{bits};

			return hll.estimate(a);
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
			return 0;
		}

		static double intersect(const uint8_t *a){
			HyperLogLogRaw hll{bits};

			return hll.estimate(a);
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
		template<typename ...Args>
		static auto merge__(Args &&...args){
		//	static_assert(sizeof...(args) >= 0 && sizeof...(args) <= 4);

			std::array<uint8_t, m> tmp;

			HyperLogLogRaw hll{bits};

			auto operations = hll.getOperations();

			return operations.merge(tmp.data(), args...);
		}

		template<typename ...Args>
		static auto intersect__(Args &&...args){
		//	static_assert(sizeof...(args) >= 0 && sizeof...(args) <= 4);

			std::array<uint8_t, m> tmp;

			HyperLogLogRaw hll{bits};

			auto operations = hll.getOperations();

			return operations.intersect(tmp.data(), args...);
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


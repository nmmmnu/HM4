#ifndef MY_VECTORS_STORAGE_H_
#define MY_VECTORS_STORAGE_H_

#include "vectors.h"
//#include "bitvectors.h"

#include "myendian.h"
#include "mybufferview.h"

namespace MyVectors{

	template<typename T>
	struct StoredVector{
		static_assert(MyVectors::checkVectorElement<T>(), "Only float, int8_t and int16_t supported");

		float		magnitudeBE;	// 4
		uint32_t	dimBE;		// 4
		T		vdata[1];	// flexible member

		constexpr static const StoredVector *createInRawMemory(void *mem, CFVector const vector){
			auto *self = static_cast<StoredVector *>(mem);

			self->dimBE = htobe( static_cast<uint32_t>(vector.size()) );

			auto normF_BE = [self](size_t const index, float const value){
				self->vdata[index] = htobe(quantizeComponent<T>(value));
			};

			self->magnitudeBE = htobe(
					normalizeF(vector, normF_BE)
			);

			return self;
		}

		constexpr auto dim() const{
			return betoh(dimBE);
		}

		constexpr auto size() const{
			return dim();
		}

		constexpr auto magnitude() const{
			return betoh(magnitudeBE);
		}

		constexpr std::string_view toSV() const{
			return std::string_view{
				reinterpret_cast<const char *>(this),
				bytes()
			};
		}

		constexpr CTVector<T> toVector() const{
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

			return { vdata, dim() };

			#pragma GCC diagnostic pop
		}

		constexpr size_t bytes() const{
			return bytes(dim());
		}

		constexpr static size_t bytes(size_t dim){
			return sizeof(StoredVector<T>) - sizeof(T) + dim * sizeof(T);
		}
	} __attribute__((__packed__));

	static_assert(std::is_standard_layout_v<StoredVector<float	> >, "Wector must be POD type");
	static_assert(std::is_standard_layout_v<StoredVector<int16_t	> >, "Wector must be POD type");
	static_assert(std::is_standard_layout_v<StoredVector<int8_t	> >, "Wector must be POD type");

/*

	template<>
	struct StoredVector<bool>{
		uint32_t	dimBE;		// 4
		uint8_t		vdata[1];	// flexible member

		static const StoredVector *createInRawMemory(void *mem, CFVector const vector){
			using namespace MyVectors;

			auto *self = static_cast<TVector<T> *>(mem);

			self->dimBE = htobe( static_cast<uint32_t>(vector.size()) );

			MyVectors::bitVectorQuantize(vector, self->vdata);

			return self;
		}

		constexpr auto dim() const{
			return betoh(dimBE);
		}

		std::string_view toSV() const{
			return std::string_view{
				reinterpret_cast<const char *>(this),
				bytes()
			};
		}

		std::string_view toBitSV() const{
			return std::string_view{
				reinterpret_cast<const char *>(vdata),
				MyVectors::bitVectorBytes(dim())
			};
		}

		bool operator[](size_t index) const{
			using namespace MyVectors;

			return bitVectorGetComponent(vdata, index);
		}

		constexpr size_t bytes() const{
			return bytes(dim());
		}

		constexpr static size_t bytes(size_t dim){
			using namespace MyVectors;

			return sizeof(Wector) - sizeof(uint8_t) + MyVectors::bitVectorBytes(dim);
		}
	} __attribute__((__packed__));

	static_assert(std::is_standard_layout_v<StoredVector<bool	> >, "Wector must be POD type");

*/

	template<typename T>
	void toStoredVector(const char *) = delete;

	template<typename T>
	void toStoredVector(const char *, uint32_t) = delete;



	template<typename T>
	const StoredVector<T> *toStoredVector(std::string_view sv){
		return reinterpret_cast<const StoredVector<T> *>(sv.data());
	}

	template<typename T>
	const StoredVector<T> *toStoredVector(std::string_view sv, uint32_t dim){
		if (sv.size() != StoredVector<T>::bytes(dim))
			return nullptr;

		if (const auto *v = toStoredVector<T>(sv); v->dim() != dim)
			return nullptr;
		else
			return v;
	}



	template<typename T, size_t Dimensions>
	using StoredTVectorBuffer = std::array<char, StoredVector<T>::bytes(Dimensions)>; // VectorBuffer + 8

	template<size_t Dimensions>
	using StoredFVectorBuffer = StoredTVectorBuffer<float, Dimensions>;

} // namespace MyStoredVectors

#endif


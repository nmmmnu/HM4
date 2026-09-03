#include "base.h"

#include "logger.h"

#include "fmt/format.h"

#include "ilist/txguard.h"

#include "mystring.h"
#include "topheap.h"

#include "vectors_storage.h"

#include "shared_rset_multi.h"

#include "shared_stoppredicate.h"
#include "shared_hashsortkey.h"
#include "shared_extractnth.h"

#include "logger.h"

#include "pair_vfactory.h"

#include <algorithm>	// clamp

namespace net::worker::commands::Vectors2{

	namespace impl_{
		// 00
		constexpr size_t keyBandSize		= 2;  // size of uint8 as hex

		// keyN~keyBand~keySort~keySub
		constexpr size_t keyAdditionalSize	= /*keyN~ */   keyBandSize + 1 + shared::sortkey::keySortSize   /* ~keySub */;



		constexpr uint32_t MaxDimensions	= 1024 * 8;

		using VectorMaxBuffer    = std::array<char, MaxDimensions * sizeof(float)>;	// 32 KB
		using VectorMaxBufferHex = std::array<char, MaxDimensions * sizeof(float) * 2>;	// 64 KB


		auto valueProjBE = [](auto const a){
			return betoh(a);
		};



		enum class QType{
			UNKNOWN	,
			F32	,
			I16	,
			I8	,
			BIT
		};

		constexpr auto translateQType(std::string_view s){
			if (s.size() != 1)
				return QType::UNKNOWN;

			switch(s[0]){
			case 'f' :
			case 'F' :	return QType::F32	;

			case 's' :
			case 'S' :	return QType::I16	;

			case 'i' :
			case 'I' :	return QType::I8	;

			case 'b' :
			case 'B' :	return QType::BIT	;

			default:	return QType::UNKNOWN	;
			}
		}

		enum class VType{
			UNKNOWN		,
			BINARY_LE	,
			BINARY_BE	,
			HEX_LE		,
			HEX_BE
		};

		constexpr auto translateVType(std::string_view s){
			if (s.size() != 1)
				return VType::UNKNOWN;

			switch(s[0]){
			case 'b' :	return VType::BINARY_LE	;
			case 'B' :	return VType::BINARY_BE	;

			case 'h' :	return VType::HEX_LE	;
			case 'H' :	return VType::HEX_BE	;

			default:	return VType::UNKNOWN	;
			}
		}

		constexpr size_t translateVTypeToSizeM(VType vtype){
			switch(vtype){
			default:
			case VType::BINARY_LE	:
			case VType::BINARY_BE	: return 1;
			case VType::HEX_LE	:
			case VType::HEX_BE	: return 2;
			}
		}



		enum class DType{
			UNKNOWN		,
			EUCLIDEAN	,
			MANHATTAN	,
			COSINE		,
			CANBERRA	,
			BIT_HAMMING	,
			BIT_COSINE	,
			BIT_DOMINATE
		};

		constexpr auto translateDType(std::string_view s){
			if (s.size() != 1)
				return DType::UNKNOWN;

			switch(s[0]){
			case 'l' :
			case 'L' :
			case 'e' :
			case 'E' :	return DType::EUCLIDEAN		;

			case 'm' :
			case 'M' :	return DType::MANHATTAN		;

			case 'c' :
			case 'C' :	return DType::COSINE		;

			case 'k' :
			case 'K' :	return DType::CANBERRA		;

			case 'h' :
			case 'H' :	return DType::BIT_HAMMING	;

			case 'b' :
			case 'B' :	return DType::BIT_COSINE	;

			case 'd' :
			case 'D' :	return DType::BIT_DOMINATE	;

			default:	return DType::UNKNOWN		;
			}
		}

		constexpr bool checkDType(DType dtype, QType qtype){
			switch(dtype){
			case DType::EUCLIDEAN		:
			case DType::MANHATTAN		:
			case DType::COSINE		:
			case DType::CANBERRA		:	return qtype != QType::BIT;

			case DType::BIT_HAMMING		:
			case DType::BIT_COSINE		:
			case DType::BIT_DOMINATE	:	return qtype == QType::BIT;

			default				:
			case DType::UNKNOWN		:	return false;
			}
		}

		template<typename T1, typename T2>
		float distance(DType dtype, MyVectors::CTVector<T1> const a, MyVectors::CTVector<T2> const b, float aM, float bM){
			using namespace MyVectors;

			switch(dtype){
			default:
			case DType::COSINE		: return MyVectors::distanceCosine		(a, b,         valueProjBE, valueProjBE);
			case DType::EUCLIDEAN		: return MyVectors::distanceEuclideanSquared	(a, b, aM, bM, valueProjBE, valueProjBE);
			case DType::MANHATTAN		: return MyVectors::distanceManhattan		(a, b, aM, bM, valueProjBE, valueProjBE);
			case DType::CANBERRA		: return MyVectors::distanceCanberra		(a, b, aM, bM, valueProjBE, valueProjBE);
			}
		}

		template<typename T2>
		float distancePrepared(DType dtype, MyVectors::CFVector const a, MyVectors::CTVector<T2> const b, float aM, float bM){
			using namespace MyVectors;

			switch(dtype){
			default:
			case DType::COSINE		: return MyVectors::distanceCosine		(a, b,         {}, valueProjBE);
			case DType::EUCLIDEAN		: return MyVectors::distanceEuclideanSquared	(a, b, aM, bM, {}, valueProjBE);
			case DType::MANHATTAN		: return MyVectors::distanceManhattanPrepared	(a, b,     bM, {}, valueProjBE);
			case DType::CANBERRA		: return MyVectors::distanceCanberraPrepared	(a, b,     bM, {}, valueProjBE);
			}
		}

		constexpr float distanceFix(DType dtype, float distance){
			switch(dtype){
			case DType::EUCLIDEAN		: return std::sqrt(distance);
			default				: return distance;
			}
		}



		constexpr float distanceBitPrepareFix(DType dtype, MyVectors::BVector a){
			switch(dtype){
			default:
			case DType::BIT_HAMMING		: return MyVectors::distanceHammingPrepareFix		(a);
			case DType::BIT_COSINE		: return MyVectors::distanceCosineBitPrepareFix		(a);
			case DType::BIT_DOMINATE	: return MyVectors::distanceDominatingPrepareFix	(a);
			}
		}

		float distanceBit(DType dtype, MyVectors::BVector a, MyVectors::BVector b){
			using namespace MyVectors;

			switch(dtype){
			default:
			case DType::BIT_HAMMING		: return MyVectors::distanceHamming		(a, b);
			case DType::BIT_COSINE		: return MyVectors::distanceCosineBit		(a, b);
			case DType::BIT_DOMINATE	: return MyVectors::distanceDominatingPrepared	(a, b);
			}
		}

		constexpr float distanceBitFix(DType dtype, float distance, float fix){
			switch(dtype){
			default:
			case DType::BIT_HAMMING		: return MyVectors::distanceHammingFix		(distance, fix);
			case DType::BIT_COSINE		: return MyVectors::distanceCosineBitFix	(distance, fix);
			case DType::BIT_DOMINATE	: return MyVectors::distanceDominatingFix	(distance, fix);
			}
		}



		template<typename T>
		auto prepareFVector(OutputBlob &blob, std::string_view data, uint32_t const dim_ix, DType dtype){
			struct Result{
				bool			ok		= false;
				MyVectors::FVector	vector		= {};
				float			magnitude	= {};
			};

			const auto *storedVector = MyVectors::toStoredVector<T>(data, dim_ix);

			if (!storedVector)
				return Result{};

			float *buffer = blob.allocateBytes<float>(dim_ix * sizeof(float) );

			MyVectors::FVector vector{ buffer, dim_ix };

			auto const magnitude = [&]() -> float{
				switch(dtype){
				default:
				case DType::COSINE	:
				case DType::EUCLIDEAN	: {

						// in:  normalized vector of T
						// out:
						//      - normalized vector of float
						//      - fixed host order

						auto f = [&vector](size_t i, float const value){
							vector[i] = value;
						};

						MyVectors::dequantizeF(storedVector->toVector(), f, valueProjBE);

						return storedVector->magnitude();
					}

				case DType::MANHATTAN	:
				case DType::CANBERRA	: {

						// in:  normalized vector of T
						// out:
						//      - denormalized vector of float
						//      - fixed host order

						auto f = [&vector](size_t i, float const value){
							vector[i] = value;
						};

						MyVectors::denormalizeF(storedVector->toVector(), storedVector->magnitude(), f, valueProjBE);

						return 1.f;
					}
				}
			}();

			return Result{
				true		,
				vector		,
				magnitude
			};
		}

		template<typename T, class DBAdapter>
		auto prepareFVector(DBAdapter &db, OutputBlob &blob, std::string_view keyN, std::string_view keySub, uint32_t const dim_ix, DType dtype){
			return prepareFVector<T>(
					blob,
					shared::rsetmulti::getData(db, keyN, keySub),
					dim_ix,
					dtype
			);
		}



		struct CreateFloatVector{
			CreateFloatVector(VType vtype, std::string_view fdata,
					uint32_t dim_ve, uint32_t dim_ix,
						VectorMaxBuffer &vectorBuffer,
						VectorMaxBuffer &vectorBufferProj) :
								vtype			(vtype			),
								fdata			(fdata			),
								dim_ve			(dim_ve			),
								dim_ix			(dim_ix			),
								vectorBuffer		(vectorBuffer		),
								vectorBufferProj	(vectorBufferProj	){}

			MyVectors::CFVector operator()(){
				auto vector = decodeToFloat(vtype, dim_ve, fdata, vectorBuffer);	// live in vectorBuffer
				fixHostOrder(vtype, vector);						// live in vectorBuffer

				bool const needsToBeProjected = dim_ix > 1 && dim_ix < dim_ve;

				if (needsToBeProjected){
					MyVectors::FVector vectorProj{ reinterpret_cast<float *>(vectorBufferProj.data()), dim_ix };

					MyVectors::randomProjection(vector, vectorProj);

					return vectorProj;						// live in vectorBufferProj !!!
				}else{
					// no projection,

					return vector;							// live in vectorBuffer
				}
			}

		private:
			static MyVectors::FVector decodeToFloat(VType type, uint32_t dim, std::string_view fdata, VectorMaxBuffer &buffer){
				switch(type){
				default:
				case VType::BINARY_LE :
				case VType::BINARY_BE : {
						// we have to memcpy() because we need to be able to return mutable version,
						// but fvectorSV is const char *

						// vectorSV size is checked already

						MyVectors::CFVector in	{ reinterpret_cast<const float *>(fdata.data()  ), dim };
						MyVectors::FVector  out	{ reinterpret_cast<      float *>(buffer.data() ), dim };

						FORCE_VECTORIZE
						for(size_t i = 0; i < dim; ++i)
							out[i] = in[i];

						return out;
					}

				case VType::HEX_LE :
				case VType::HEX_BE : {
						// vectorSV size is checked already

						hex_convert::fromHexBytes(fdata, buffer);

						return MyVectors::FVector{ reinterpret_cast<float *>(buffer.data()), dim };
					}
				}
			}

			static void fixHostOrder(VType type, MyVectors::FVector &vector){
				switch(type){
				default:
				case VType::BINARY_LE :
				case VType::HEX_LE    : {

						FORCE_VECTORIZE
						for(size_t i = 0; i < vector.size(); ++i)
							vector[i] = letoh(vector[i]);

						return;
					}

				case VType::BINARY_BE :
				case VType::HEX_BE    : {

						FORCE_VECTORIZE
						for(size_t i = 0; i < vector.size(); ++i)
							vector[i] = betoh(vector[i]);

						return;
					}
				}
			}

		private:
			VType			vtype	;
			std::string_view	fdata	;
			uint32_t		dim_ve	;
			uint32_t		dim_ix	;

		private:
			VectorMaxBuffer		&vectorBuffer		;
			VectorMaxBuffer		&vectorBufferProj	;
		};



		constexpr size_t hashEncodeSize = 8;

		constexpr std::string_view hashEncode(uint16_t val, char *buffer){
			constexpr char dot   = '.';

			constexpr char bin[] = "01";
			constexpr char hex[] = "0123456789ABCDEF";

			buffer[0] = hex[ (val >> (4 * 3)) & 0x0F ];
			buffer[1] = hex[ (val >> (4 * 2)) & 0x0F ];
			buffer[2] = hex[ (val >> (4 * 1)) & 0x0F ];

			buffer[3] = dot;

			buffer[4] = (val & (1 << 3)) ? bin[1] : bin[0];
			buffer[5] = (val & (1 << 2)) ? bin[1] : bin[0];
			buffer[6] = (val & (1 << 1)) ? bin[1] : bin[0];
			buffer[7] = (val & (1 << 0)) ? bin[1] : bin[0];

			return std::string_view(buffer, hashEncodeSize);
		}

	//	template<size_t N>
	//	constexpr std::string_view hashEncode(uint16_t val, std::array<char, N> &buffer) {
	//		static_assert(N >= hashEncodeSize); // buffer is ABC.0000
	//
	//		return hashEncode(val, buffer.data());
	//	}



		template<typename T>
		struct Decoder{
			Decoder(uint32_t dim_ix) :	dim_ix	(dim_ix			){}

			Decoder(uint32_t dim_ix, size_t bands, size_t bits) :
							dim_ix	(dim_ix			),
							bands	(fixBands(bands)	),
							bits	(fixBits (bits )	){}

			constexpr auto bytes() const{
				return MyVectors::StoredVector<T>::bytes(dim_ix);
			}

			template<typename IContainer, typename BContainer>
			bool operator()(std::string_view data,
						IContainer &icontainer, BContainer &bcontainer) const{

				icontainer.clear();
				bcontainer.clear();

				// size will be checked in a moment

				const auto *storedVector = MyVectors::toStoredVector<T>(data, dim_ix);

				if (!storedVector)
					return false;

				auto f = [this, &icontainer, &bcontainer](size_t id64, band_type hash){
					assert(id64 < MAX_BANDS);

					uint8_t const id = static_cast<uint8_t>(id64);

					bcontainer.push_back();

					char *buffer = bcontainer.back().data();

					size_t p = 0;

					// ABC.0000.05

					hex_convert::toHex(id, buffer + p);	p += sizeof(uint8_t) * 2;
					buffer[p] = '.';			p += 1;
					hashEncode(hash, buffer + p);		p += hashEncodeSize;

					auto const skip = MAX_BITS - bits; // if 12 => 4, if 16 => 0

					icontainer.emplace_back(buffer, p - skip);
				};

				if constexpr(!std::is_same_v<T, bool>){
					MyVectors::simhashBands		<MaxDimensions, band_type>(storedVector->toVector(), bands, f, valueProjBE);
				}else{
					MyVectors::simhashBandsBit	<MaxDimensions, band_type>(storedVector->toVector(), bands, f);
				}

				return true;
			}

			constexpr static size_t fixBits(size_t bits){
				if (bits == 0)
					return MAX_BITS;
				else
					return std::clamp<size_t>(bits, MIN_BITS, MAX_BITS);
			}

			constexpr static size_t fixBands(size_t bands){
				if (bands == 0)
					return MAX_BANDS;
				else
					return std::clamp<size_t>(bands, 1, MAX_BANDS);
			}

		public:
			constexpr static size_t MAX_BANDS = 256; // 00 to FF

		public:
			using IContainer	= OutputBlob::TContainer	<MAX_BANDS>;
			using BContainer	= OutputBlob::TBufferContainer	<MAX_BANDS>;

			using band_type		= uint16_t;

		private:
			constexpr static size_t MIN_BITS  =  12;
			constexpr static size_t MAX_BITS  =  16;

		private:
			uint32_t	dim_ix;
			size_t		bands	= fixBands(0);
			size_t		bits	= fixBits (0);
		};



		template<size_t N>
		std::string_view formatDouble(float d, std::array<char, N> &buffer){
			constexpr static std::string_view fmt_mask = "{:+015.15f}";

			auto const result = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask, d);

			if (result.out == std::end(buffer))
				return {};
			else
				return { buffer.data(), result.size };
		}

		// VGET key   DIM_IX QUANTIZE_TYPE name
		// VGET words 150    F             frog
		// VGET words 150    I             frog

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		S = int16
		I = int8
		B = bit
		*/

		template<typename T, bool Norm, typename Protocol, typename DBAdapter>
		void process__VGET_(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob, std::string_view keyN, std::string_view keySub, uint32_t dim_ix);

		template<bool Norm, typename Protocol, typename DBAdapter>
		void process__VGET(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const &keyN   = p[1];
			auto const &keySub = p[4];

			if (keyN.empty() || keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!shared::rsetmulti::valid(keyN, keySub, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const dim_ix = from_string<uint32_t>(p[2]);

			if (dim_ix < 1 || dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[3]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			switch(qtype){
			case QType::F32 : return process__VGET_<float	, Norm>(db, result, blob, keyN, keySub, dim_ix);
			case QType::I16 : return process__VGET_<int16_t	, Norm>(db, result, blob, keyN, keySub, dim_ix);
			case QType::I8  : return process__VGET_<int8_t	, Norm>(db, result, blob, keyN, keySub, dim_ix);
			case QType::BIT : return process__VGET_<bool	, Norm>(db, result, blob, keyN, keySub, dim_ix);

			default		: return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		template<typename T, bool Norm, typename Protocol, typename DBAdapter>
		void process__VGET_(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob, std::string_view keyN, std::string_view keySub, uint32_t dim_ix){
			auto const sv = shared::rsetmulti::getData(db, keyN, keySub);

			const auto *storedVector = MyVectors::toStoredVector<T>(sv, dim_ix);

			if (!storedVector)
				return result.set(false);

			if constexpr(!std::is_same_v<T, bool>){

				auto const vector	= storedVector->toVector();
				auto const magnitude	= storedVector->magnitude();

				auto &container		= blob.construct<OutputBlob::Container>();
				auto &bcontainer	= blob.construct<OutputBlob::BufferContainer>();

				if constexpr(Norm){
					bcontainer.push_back();

					auto const s = formatDouble(magnitude, bcontainer.back());

					container.push_back(s);
				}

				auto f = [&container, &bcontainer, magnitude](size_t, float const value){
					bcontainer.push_back();

					if constexpr(Norm){
						auto const s = formatDouble(value,             bcontainer.back());

						container.push_back(s);

						// printf("%+8.4f %+5d\n", value, MyVectors::quantizeComponentToI8(value));

						// clang
						(void) magnitude;
					}else{
						auto const s = formatDouble(value * magnitude, bcontainer.back());

						container.push_back(s);
					}
				};

				MyVectors::dequantizeF(vector, f, valueProjBE);

				return result.set_container(container);
			}else{
				auto const vector	= storedVector->toVector();

				auto &container		= blob.construct<OutputBlob::Container>();

				if constexpr(Norm){
					container.push_back("0");
				}

				std::string_view const fp = "+1";
				std::string_view const fn = "-1";

				for (size_t i = 0; i < dim_ix; ++i)
					container.push_back(vector[i] ? fp : fn);

				return result.set_container(container);
			}
		}

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct VADD : BaseCommandRW<Protocol,DBAdapter>{

		VADD() : BaseCommandRW<Protocol,DBAdapter>("VADD", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// VADD key   DIM_VE DIM_IX QUANTIZE_TYPE VEC_TYPE name BLOB  name BLOB  ...
		// VADD words 300    150    F             b        cat  BLOB0 frog BLOB1
		// VADD words 300    150    I             b        cat  BLOB0 frog BLOB1

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		S = int16
		I = int8
		B = bit

		VEC_TYPE:
		B = binary BE
		b = binary LE
		H = hex BE
		h = hex LE
		*/

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			auto const varg  = 6;
			auto const vstep = 2;

			if (p.size() < varg + vstep || (p.size() - varg) % vstep != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_6);

			auto const keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const dim_ve = from_string<uint32_t>(p[2]);

			if (dim_ve <  1 || dim_ve > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const dim_ix = from_string<uint32_t>(p[3]);

			if (dim_ix < 1 || dim_ix > dim_ve)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[4]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const vtype = translateVType(p[5]);

			if (vtype == VType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const sizeM = translateVTypeToSizeM(vtype);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const keySub   = *(itk + 0);

				if (keySub.empty())
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				if (!shared::rsetmulti::valid(keyN, keySub, keyAdditionalSize))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

				auto const vectorSV = *(itk + 1);

				if (!MyVectors::validBlobSizeF(vectorSV.size(), dim_ve * sizeM))
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}

			switch(qtype){
			case QType::F32	: return process__<float	>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, dim_ve, dim_ix, vtype);
			case QType::I16	: return process__<int16_t	>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, dim_ve, dim_ix, vtype);
			case QType::I8	: return process__<int8_t	>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, dim_ve, dim_ix, vtype);
			case QType::BIT	: return process__<bool		>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, dim_ve, dim_ix, vtype);

			default		: return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		using IT = ParamContainer::iterator;

		template<typename T>
		static void process__(IT first, IT last,
				DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
					std::string_view keyN, uint32_t const dim_ve, uint32_t const dim_ix, impl_::VType vtype){

			using namespace impl_;

			auto const vstep = 2;

			auto &vectorBuffer	= blob.allocate<VectorMaxBuffer>();
			auto &vectorBufferProj	= blob.allocate<VectorMaxBuffer>();

			using MyDecoder		= Decoder<T>;

			auto &icontainer	= blob.construct<typename MyDecoder::IContainer>();
			auto &bcontainer	= blob.construct<typename MyDecoder::BContainer>();

			MyDecoder decoder{ dim_ix };

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			for(auto itk = first; itk != last; itk += vstep){
				auto const keySub   = *(itk + 0);
				auto const vectorSV = *(itk + 1);

				MyVectors::CFVector cfvector = CreateFloatVector{ vtype, vectorSV,
							dim_ve, dim_ix,
								vectorBuffer, vectorBufferProj }();

				to_string_buffer_t buffer;
				auto const keySort  = shared::sortkey::makeHashKeySort(keySub, buffer);

				VADD_Factory<T> factory{ cfvector, decoder, icontainer, bcontainer };

				[[maybe_unused]]
				bool const b = shared::rsetmulti::add(db, decoder,
								keyN, keySub, keySort,
									icontainer, bcontainer,
										factory);
			}

			return result.set();
		}



		template<typename T>
		struct VADD_Factory : hm4::PairFactory::IFactoryAction<0, 0, VADD_Factory<T> >{
			using Pair  = hm4::Pair;
			using Base  = hm4::PairFactory::IFactoryAction<0, 0, VADD_Factory<T> >;

			using MyDecoder = impl_::Decoder<T>;

			constexpr VADD_Factory(MyVectors::CFVector cfvector,
								MyDecoder decoder,
									typename MyDecoder::IContainer &icontainer,
									typename MyDecoder::BContainer &bcontainer) :
							Base::IFactoryAction	(/* key */ {}, decoder.bytes() ),
							cfvector		(cfvector	),
							decoder			(decoder	),
							icontainer		(icontainer	),
							bcontainer		(bcontainer	){}

			void action(Pair *pair){
				return action_(pair);
			}

			auto const &getIndexes() const{
				return icontainer;
			}

		private:
			void action_(Pair *pair) const{
				auto *mem = hm4::getValAs<MyVectors::StoredVector<T> >(pair);

				MyVectors::StoredVector<T>::createInRawMemory(mem, cfvector);

				decoder(pair->getVal(), icontainer, bcontainer);
			}

		private:
			MyVectors::CFVector		cfvector;
			MyDecoder 			decoder;
			typename MyDecoder::IContainer	&icontainer;
			typename MyDecoder::BContainer	&bcontainer;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"vadd",	"VADD"
		};

	};



	template<class Protocol, class DBAdapter>
	struct VREM : BaseCommandRW<Protocol,DBAdapter>{

		VREM() : BaseCommandRW<Protocol,DBAdapter>("VREM", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// VREM key   DIM_IX QUANTIZE_TYPE name name ...
		// VREM words 150    F             frog cat
		// VREM words 150    I             frog cat

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		S = int16
		I = int8
		B = bit
		*/

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			auto const varg  = 4;

			if (p.size() < 5)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_5);

			auto const keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const dim_ix = from_string<uint32_t>(p[2]);

			if (dim_ix < 1 || dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[3]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const keySub = *itk;

				if (keySub.empty())
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				if (!shared::rsetmulti::valid(keyN, keySub, keyAdditionalSize))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			switch(qtype){
			case QType::F32	: return process__<float	>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, dim_ix);
			case QType::I16	: return process__<int16_t	>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, dim_ix);
			case QType::I8	: return process__<int8_t	>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, dim_ix);
			case QType::BIT	: return process__<bool		>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, dim_ix);

			default		: return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		using IT = ParamContainer::iterator;

		template<typename T>
		static void process__(IT first, IT last, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob, std::string_view keyN, uint32_t const dim_ix){
			using namespace impl_;

			using MyDecoder		= Decoder<T>;

			auto &icontainer	= blob.construct<typename MyDecoder::IContainer>();
			auto &bcontainer	= blob.construct<typename MyDecoder::BContainer>();

			MyDecoder decoder{ dim_ix };

			for(auto itk = first; itk != last; ++itk){
				auto const keySub	= *itk;

				to_string_buffer_t buffer;
				auto const keySort	= shared::sortkey::makeHashKeySort(keySub, buffer);

				[[maybe_unused]]
				bool const b = shared::rsetmulti::rem(db, decoder,
								keyN, keySub, keySort,
									icontainer, bcontainer);
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"vrem",		"VREM"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VGETINDEXES : BaseCommandRO<Protocol,DBAdapter>{

		VGETINDEXES() : BaseCommandRO<Protocol,DBAdapter>("VGETINDEXES", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// VGETINDEXES key   DIM_IX QUANTIZE_TYPE name
		// VGETINDEXES words 150    F             frog
		// VGETINDEXES words 150    I             frog

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		S = int16
		I = int8
		B = bit
		*/

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const &keyN   = p[1];
			auto const &keySub = p[4];

			if (keyN.empty() || keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!shared::rsetmulti::valid(keyN, keySub, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const dim_ix = from_string<uint32_t>(p[2]);

			if (dim_ix < 1 || dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[3]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			switch(qtype){
			case QType::F32 : return process__<float	>(db, result, blob, keyN, keySub, dim_ix);
			case QType::I16 : return process__<int16_t	>(db, result, blob, keyN, keySub, dim_ix);
			case QType::I8  : return process__<int8_t	>(db, result, blob, keyN, keySub, dim_ix);
			case QType::BIT : return process__<bool		>(db, result, blob, keyN, keySub, dim_ix);

			default		: return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		template<typename T>
		static void process__(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob, std::string_view keyN, std::string_view keySub, uint32_t dim_ix){
			using namespace impl_;

			using MyDecoder		= Decoder<T>;

			auto &icontainer	= blob.construct<typename MyDecoder::IContainer>();
			auto &bcontainer	= blob.construct<typename MyDecoder::BContainer>();

			MyDecoder decoder{ dim_ix };

			[[maybe_unused]]
			bool const b = shared::rsetmulti::getIndexes(db, decoder,
							keyN, keySub,
								icontainer, bcontainer);

			auto const &container = icontainer;

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"vgetindexes",	"VGETINDEXES"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VGET : BaseCommandRO<Protocol,DBAdapter>{

		VGET() : BaseCommandRO<Protocol,DBAdapter>("VGET", std::begin(cmd__), std::end(cmd__)){}

		// VGET key   DIM_IX QUANTIZE_TYPE name
		// VGET words 150    F             frog
		// VGET words 150    I             frog

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		S = int16
		I = int8
		B = bit
		*/

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			return process__VGET<0>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"vget",		"VGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VGETNORMALIZED : BaseCommandRO<Protocol,DBAdapter>{

		VGETNORMALIZED() : BaseCommandRO<Protocol,DBAdapter>("VGETNORMALIZED", std::begin(cmd__), std::end(cmd__)){}

		// VGETNORMALIZED key   DIM_IX QUANTIZE_TYPE name
		// VGETNORMALIZED words 150    F             frog
		// VGETNORMALIZED words 150    I             frog

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		S = int16
		I = int8
		B = bit
		*/

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			return process__VGET<1>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"vgetnormalized",		"VGETNORMALIZED",
			"vgetnorm",			"VGETNORM"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VGETRAW : BaseCommandRO<Protocol,DBAdapter>{

		VGETRAW() : BaseCommandRO<Protocol,DBAdapter>("VGETRAW", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// VGETRAW key   DIM_IX QUANTIZE_TYPE OUT_VEC_TYPE name
		// VGETRAW words 300    F             b            frog
		// VGETRAW words 300    I             b            frog

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		S = int16
		I = int8
		B = bit

		VEC_TYPE:
		B = binary BE
		b = binary LE
		H = hex BE
		h = hex LE
		*/

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 6)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);

			auto const &keyN   = p[1];
			auto const &keySub = p[5];

			if (keyN.empty() || keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!shared::rsetmulti::valid(keyN, keySub, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const dim_ix = from_string<uint32_t>(p[2]);

			if (dim_ix < 1 || dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[3]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const vtype = translateVType(p[4]);

			if (vtype == VType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			switch(qtype){
			case QType::F32 : return process__<float	>(db, result, blob, keyN, keySub, dim_ix, vtype);
			case QType::I16 : return process__<int16_t	>(db, result, blob, keyN, keySub, dim_ix, vtype);
			case QType::I8  : return process__<int8_t	>(db, result, blob, keyN, keySub, dim_ix, vtype);
			case QType::BIT : return process__<bool		>(db, result, blob, keyN, keySub, dim_ix, vtype);

			default		: return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		template<typename T>
		static void process__(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob, std::string_view keyN, std::string_view keySub, uint32_t dim_ix, impl_::VType vtype){
			using namespace impl_;

			auto const sv = shared::rsetmulti::getData(db, keyN, keySub);

			const auto *storedVector = MyVectors::toStoredVector<T>(sv, dim_ix);

			if (!storedVector)
				return result.set(false);

			switch(vtype){
			default:
			case VType::BINARY_LE :
			case VType::BINARY_BE : {
					auto &fVectorBuffer = blob.allocate<VectorMaxBuffer>();

					MyVectors::FVector fvector{ reinterpret_cast<float *>(fVectorBuffer.data()), dim_ix };

					if constexpr(!std::is_same_v<T, bool>){

						auto const vector	= storedVector->toVector();
						auto const magnitude	= storedVector->magnitude();

						auto f = [&fvector](size_t i, float const value){
							fvector[i] = value;
						};

						MyVectors::denormalizeF(vector, magnitude, f, valueProjBE);

						// fvector is denormalized and in host order now.

						if (vtype == VType::BINARY_BE){
							FORCE_VECTORIZE
							for(size_t i = 0; i < dim_ix; ++i)
								fvector[i] = htobe(fvector[i]);
						}else{
							FORCE_VECTORIZE
							for(size_t i = 0; i < dim_ix; ++i)
								fvector[i] = htole(fvector[i]);
						}
					}else{
						auto const vector	= storedVector->toVector();

						float const fp = vtype == VType::BINARY_BE ? htobe(+1.f) : htole(+1.f);
						float const fn = vtype == VType::BINARY_BE ? htobe(-1.f) : htole(-1.f);

						for (size_t i = 0; i < dim_ix; ++i)
							fvector[i] = vector[i] ? fp : fn;
					}

					return result.set(
						std::string_view{
							fVectorBuffer.data(),
							dim_ix * sizeof(float)
						}
					);
				}

			case VType::HEX_LE :
			case VType::HEX_BE : {

					auto &bufferHex = blob.construct<VectorMaxBufferHex>();

					// however our toHex() is big endian, so we have to negate

					if (vtype != VType::HEX_BE){
						// big endian data

						auto f_be = [&bufferHex](size_t i, float const value_){
							float const value = htobe(value_);

							uint32_t const u32 = bit_cast<uint32_t>(value);

							char *buff = bufferHex.data() + i * sizeof(float) * 2;

							hex_convert::toHex(u32, buff);
						};

						if constexpr(!std::is_same_v<T, bool>){
							auto const vector	= storedVector->toVector();
							auto const magnitude	= storedVector->magnitude();

							MyVectors::denormalizeF(vector, magnitude, f_be, valueProjBE);
						}else{
							auto const vector	= storedVector->toVector();

							float const fp = +1.f;
							float const fn = -1.f;

							for (size_t i = 0; i < dim_ix; ++i)
								f_be(i, vector[i] ? fp : fn);
						}
					}else{
						// little endian data

						auto f_le = [&bufferHex](size_t i, float const value_){
							float const value = htole(value_);

							uint32_t const u32 = bit_cast<uint32_t>(value);

							char *buff = bufferHex.data() + i * sizeof(float) * 2;

							hex_convert::toHex(u32, buff);
						};

						if constexpr(!std::is_same_v<T, bool>){
							auto const vector	= storedVector->toVector();
							auto const magnitude	= storedVector->magnitude();

							MyVectors::denormalizeF(vector, magnitude, f_le, valueProjBE);
						}else{
							auto const vector	= storedVector->toVector();

							float const fp = +1.f;
							float const fn = -1.f;

							for (size_t i = 0; i < dim_ix; ++i)
								f_le(i, vector[i] ? fp : fn);
						}
					}

					return result.set(
						std::string_view{
							bufferHex.data(),
							dim_ix * sizeof(float) * 2
						}
					);
				}
			} // switch vtype
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"vgetraw",	"VGETRAW",
			"vgetblob",	"VGETBLOB"
		};

	};



	template<class Protocol, class DBAdapter>
	struct VDISTANCE : BaseCommandRO<Protocol,DBAdapter>{

		VDISTANCE() : BaseCommandRO<Protocol,DBAdapter>("VDISTANCE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		// VDISTANCE key   DIM_IX QUANTIZE_TYPE DISTANCE_TYPE nameA nameB
		// VDISTANCE words 150    F             C             frog  cat
		// VDISTANCE words 150    I             E             frog  cat

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		S = int16
		I = int8
		B = bit

		DISTANCE_TYPE:
		E = Euclidean L2
		M = Manhattan L1
		C = Cosine
		K = Canberra

		DISTANCE_TYPE for bit vectors:
		H = Hamming
		B = Bit Cosine
		D = Dominate
		*/

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			using namespace impl_;

			if (p.size() != 7)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_6);

			auto const keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const dim_ix = from_string<uint32_t>(p[2]);

			if (dim_ix < 1 || dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[3]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const dtype = translateDType(p[4]);

			if (!checkDType(dtype, qtype))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const keySubA = p[5];
			auto const keySubB = p[6];

			if (keySubA.empty() || keySubB.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!shared::rsetmulti::valid(keyN, keySubA, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			if (!shared::rsetmulti::valid(keyN, keySubB, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			switch(qtype){
			case QType::F32	: return process__<float	>(db, result, keyN, keySubA, keySubB, dim_ix, dtype);
			case QType::I16	: return process__<int16_t	>(db, result, keyN, keySubA, keySubB, dim_ix, dtype);
			case QType::I8	: return process__<int8_t	>(db, result, keyN, keySubA, keySubB, dim_ix, dtype);
			case QType::BIT	: return process__<bool		>(db, result, keyN, keySubA, keySubB, dim_ix, dtype);

			default		: return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		template<typename T>
		static void process__(DBAdapter &db, Result<Protocol> &result,
								std::string_view keyN, std::string_view keySubA, std::string_view keySubB, uint32_t const dim_ix, impl_::DType dtype){
			using namespace impl_;

			std::string_view svA = shared::rsetmulti::getData(db, keyN, keySubA);

			const auto *storedVectorA = MyVectors::toStoredVector<T>(svA, dim_ix);

			if (!storedVectorA)
				return result.set("INF");

			std::string_view svB = shared::rsetmulti::getData(db, keyN, keySubB);

			const auto *storedVectorB = MyVectors::toStoredVector<T>(svB, dim_ix);

			if (!storedVectorB)
				return result.set("INF");

			if constexpr(!std::is_same_v<T, bool>){
				float const dist = distanceFix(dtype,
							distance(dtype,
								storedVectorA->toVector(),	storedVectorB->toVector(),
								storedVectorA->magnitude(),	storedVectorB->magnitude()
							)
				);

				to_string_buffer_t buffer;

				return result.set(formatDouble(dist, buffer));
			}else{
				float const fix = distanceBitPrepareFix(dtype, storedVectorA->toVector());

				float const dist = distanceBitFix(dtype,
							distanceBit(dtype,
								storedVectorA->toVector(),	storedVectorB->toVector()
							), fix
				);

				to_string_buffer_t buffer;

				return result.set(formatDouble(dist, buffer));
			}
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"vdistance"	,	"VDISTANCE"	,
			"vdist"		,	"VDIST"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VMDISTANCE : BaseCommandRO<Protocol,DBAdapter>{

		VMDISTANCE() : BaseCommandRO<Protocol,DBAdapter>("VMDISTANCE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// VDISTANCE key   DIM_IX QUANTIZE_TYPE DISTANCE_TYPE nameA name...
		// VDISTANCE words 150    F             C             frog  cat
		// VDISTANCE words 150    I             E             frog  cat

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		S = int16
		I = int8
		B = bit

		DISTANCE_TYPE:
		E = Euclidean L2
		M = Manhattan L1
		C = Cosine
		K = Canberra

		DISTANCE_TYPE for bit vectors:
		H = Hamming
		B = Bit Cosine
		D = Dominate
		*/

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			auto const varg  = 6;

			if (p.size() < 7)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_6);

			auto const keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const dim_ix = from_string<uint32_t>(p[2]);

			if (dim_ix < 1 || dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[3]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const dtype = translateDType(p[4]);

			if (!checkDType(dtype, qtype))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const keySubA = p[5];

			if (keySubA.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!shared::rsetmulti::valid(keyN, keySubA, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const keySub = *itk;

				if (keySub.empty())
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				if (!shared::rsetmulti::valid(keyN, keySub, keyAdditionalSize))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			switch(qtype){
			case QType::F32	: return process__<float	>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, keySubA, dim_ix, dtype);
			case QType::I16	: return process__<int16_t	>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, keySubA, dim_ix, dtype);
			case QType::I8	: return process__<int8_t	>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, keySubA, dim_ix, dtype);
			case QType::BIT	: return process__<bool		>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, keySubA, dim_ix, dtype);

			default		: return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		using IT = ParamContainer::iterator;

		template<typename T>
		static void process__(IT first, IT last, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
								std::string_view keyN, std::string_view keySubA, uint32_t const dim_ix, impl_::DType dtype){
			if (std::distance(first, last) <= 2)
				process__naive__<T>
						(first, last, db, result, blob,
							keyN, keySubA, dim_ix, dtype);
			else
				process__prepared__<T>
						(first, last, db, result, blob,
							keyN, keySubA, dim_ix, dtype);
		}

		template<typename T>
		static void process__naive__(IT first, IT last, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
								std::string_view keyN, std::string_view keySubA, uint32_t const dim_ix, impl_::DType dtype){
			using namespace impl_;

			auto const sv = shared::rsetmulti::getData(db, keyN, keySubA);

			const auto *storedVectorA = MyVectors::toStoredVector<T>(sv, dim_ix);

			if (!storedVectorA){
				auto &container = blob.construct<OutputBlob::SmallContainer>();

				for(auto itk = first; itk != last; ++itk)
					container.push_back(INF);

				return result.set_container(container);
			}

			auto &container  = blob.construct<OutputBlob::SmallContainer>();
			auto &bcontainer = blob.construct<OutputBlob::SmallBufferContainer>();

			for(auto itk = first; itk != last; ++itk){
				auto const keySubB	= *itk;

				auto const sv = shared::rsetmulti::getData(db, keyN, keySubB);

				const auto *storedVectorB = MyVectors::toStoredVector<T>(sv, dim_ix);

				if (!storedVectorB){
					container.push_back(INF);
					continue;
				}

				float const dist = distanceFix(dtype,
						distance(dtype,
							storedVectorA->toVector(),	storedVectorB->toVector(),
							storedVectorA->magnitude(),	storedVectorB->magnitude()
						)
					);


				bcontainer.push_back();

				container.push_back(formatDouble(dist, bcontainer.back()));
			}

			return result.set_container(container);
		}

		template<typename T>
		static void process__prepared__(IT first, IT last, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
								std::string_view keyN, std::string_view keySubA, uint32_t const dim_ix, impl_::DType dtype){
			using namespace impl_;

			auto const pp = prepareFVector<T>(db, blob, keyN, keySubA, dim_ix, dtype);

			if (!pp.ok){
				auto &container = blob.construct<OutputBlob::SmallContainer>();

				for(auto itk = first; itk != last; ++itk)
					container.push_back(INF);

				return result.set_container(container);
			}

			auto &container  = blob.construct<OutputBlob::SmallContainer>();
			auto &bcontainer = blob.construct<OutputBlob::SmallBufferContainer>();

			auto const vectorPrepared	= pp.vector;
			auto const magnitudePrepared	= pp.magnitude;

			for(auto itk = first; itk != last; ++itk){
				auto const keySubB	= *itk;

				std::string_view sv = shared::rsetmulti::getData(db, keyN, keySubB);

				const auto *storedVectorB = MyVectors::toStoredVector<T>(sv, dim_ix);

				if (!storedVectorB){
					container.push_back(INF);
					continue;
				}

				float const dist = distanceFix(dtype,
						distancePrepared(dtype,
							vectorPrepared,		storedVectorB->toVector(),
							magnitudePrepared,	storedVectorB->magnitude()
						)
					);


				bcontainer.push_back();

				container.push_back(formatDouble(dist, bcontainer.back()));
			}

			return result.set_container(container);
		}

		template<>
		/* static */ void process__<bool>(IT first, IT last, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
								std::string_view keyN, std::string_view keySubA, uint32_t const dim_ix, impl_::DType dtype){
			using namespace impl_;

			std::string_view sv = shared::rsetmulti::getData(db, keyN, keySubA);

			const auto *storedVectorA = MyVectors::toStoredVector<bool>(sv, dim_ix);

			if (!storedVectorA){
				auto &container = blob.construct<OutputBlob::SmallContainer>();

				for(auto itk = first; itk != last; ++itk)
					container.push_back(INF);

				return result.set_container(container);
			}

			auto &container  = blob.construct<OutputBlob::SmallContainer>();
			auto &bcontainer = blob.construct<OutputBlob::SmallBufferContainer>();

			float const fix = distanceBitPrepareFix(dtype, storedVectorA->toVector());

			for(auto itk = first; itk != last; ++itk){
				auto const keySubB = *itk;

				std::string_view sv = shared::rsetmulti::getData(db, keyN, keySubB);

				const auto *storedVectorB = MyVectors::toStoredVector<bool>(sv, dim_ix);

				if (!storedVectorB){
					container.push_back(INF);
					continue;
				}

				float const dist = distanceBitFix(dtype,
							distanceBit(dtype,
								storedVectorA->toVector(),
								storedVectorB->toVector()
							), fix
				);

				bcontainer.push_back();

				container.push_back(formatDouble(dist, bcontainer.back()));
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view INF = "+inf";

	private:
		constexpr inline static std::string_view cmd__[] = {
			"vmdistance"	,	"VMDISTANCE"	,
			"vmdist"	,	"VMDIST"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VSIMFLAT : BaseCommandRO<Protocol,DBAdapter>{

		VSIMFLAT() : BaseCommandRO<Protocol,DBAdapter>("VSIMFLAT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// VSIMFLAT key   DIM_IX QUANTIZE_TYPE DISTANCE_TYPE name START
		// VSIMFLAT words 300    F             C             frog ''
		// VSIMFLAT words 300    I             C             frog dfssdhg

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		S = int16
		I = int8
		B = bit

		VEC_TYPE:
		B = binary BE
		b = binary LE
		H = hex BE
		h = hex LE

		DISTANCE_TYPE:
		E = Euclidean L2
		M = Manhattan L1
		C = Cosine
		K = Canberra

		DISTANCE_TYPE for bit vectors:
		H = Hamming
		B = Bit Cosine
		D = Dominate
		*/

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 7)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_6);

			auto const keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const dim_ix = from_string<uint32_t>(p[2]);

			if (dim_ix < 1 || dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[3]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const dtype = translateDType(p[4]);

			if (!checkDType(dtype, qtype))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const keySub = p[5];

			if (keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!shared::rsetmulti::valid(keyN, keySub, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const startKey = p[6];

			switch(qtype){
			case QType::F32	: return process__<float	>(db, result, blob, keyN, keySub, dim_ix, dtype, startKey);
			case QType::I16	: return process__<int16_t	>(db, result, blob, keyN, keySub, dim_ix, dtype, startKey);
			case QType::I8	: return process__<int8_t	>(db, result, blob, keyN, keySub, dim_ix, dtype, startKey);
			case QType::BIT	: return process__<bool		>(db, result, blob, keyN, keySub, dim_ix, dtype, startKey);

			default		: return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		template<typename T>
		static void process__(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
						std::string_view keyN, std::string_view keySub,
							uint32_t const dim_ix, impl_::DType dtype,
								std::string_view startKey){
			using namespace impl_;

			auto const pp = prepareFVector<T>(db, blob, keyN, keySub, dim_ix, dtype);

			if (!pp.ok)
				return result.set_container0();

			auto const vectorPrepared	= pp.vector;
			auto const magnitudePrepared	= pp.magnitude;

			auto &heap = blob.construct<VSIMHeap>();

			hm4::PairBufferKey bufferKey;

			auto const prefix = shared::rsetmulti::makeKeyDataSearchFlat(bufferKey, DBAdapter::SEPARATOR, keyN);

			shared::stop_predicate::StopPrefixPredicate stop{ prefix };

			logger<Logger::DEBUG>() << "VSIM.FLAT" << "range prefix" << prefix << "start key" << (startKey.empty() ? "(none)" : startKey);

			if (startKey.empty()){
				startKey = prefix;
				logger<Logger::DEBUG>() << "VSIM.FLAT" << "start key change" << startKey;
			}

			uint32_t iterations = 0;

			auto const tail = process_range__<T>(
						db,
						stop,
						startKey,
						keyN, keySub,
						dim_ix,
						dtype,
						vectorPrepared, magnitudePrepared,
						heap,
						iterations
			);

			logger<Logger::DEBUG>() << "VSIM.FLAT" << "range finish" << iterations << "vectors";

			return process_range_finish__<T>(db, result, blob, dtype, heap, tail);
		}

		template<>
		/* static */ void process__<bool>(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
						std::string_view keyN, std::string_view keySub,
							uint32_t const dim_ix, impl_::DType dtype,
								std::string_view startKey){
			using namespace impl_;

			std::string_view sv = shared::rsetmulti::getData(db, keyN, keySub);

			const auto *storedVectorA = MyVectors::toStoredVector<bool>(sv, dim_ix);

			if (!storedVectorA)
				return result.set_container0();

			auto const vectorA = storedVectorA->toVector();

			auto const fix = distanceBitPrepareFix(dtype, vectorA);

			auto &heap = blob.construct<VSIMHeap>();

			hm4::PairBufferKey bufferKey;

			auto const prefix = shared::rsetmulti::makeKeyDataSearchFlat(bufferKey, DBAdapter::SEPARATOR, keyN);

			shared::stop_predicate::StopPrefixPredicate stop{ prefix };

			logger<Logger::DEBUG>() << "VSIM.FLAT.BIN" << "range prefix" << prefix << "start key" << (startKey.empty() ? "(none)" : startKey);

			if (startKey.empty()){
				startKey = prefix;
				logger<Logger::DEBUG>() << "VSIM.FLAT.BIN" << "start key change" << startKey;
			}

			uint32_t iterations = 0;

			auto const tail = process_rangeBit__(
						db,
						stop,
						startKey,
						keyN, keySub,
						dim_ix,
						dtype,
						vectorA,
						heap,
						iterations
			);

			logger<Logger::DEBUG>() << "VSIM.FLAT.BIN" << "range finish" << iterations << "vectors";

			return process_rangeBit_finish__(db, result, blob, dtype, fix, heap, tail);
		}


	private:
		template<typename T, typename StopPredicate, typename VSIM_Heap>
		static std::string_view process_range__(DBAdapter &db,
					StopPredicate predicate,
					std::string_view startKey,
					std::string_view /* keyN */, std::string_view keySub,
						uint32_t const dim_ix, impl_::DType dtype,
							MyVectors::CFVector const vectorPreparedA, float const magnitudePreparedA,
								/* VSIMHeap */ VSIM_Heap &heap, uint32_t &iterations){

			static_assert(std::is_same_v<VSIM_Heap, VSIMHeap>);

			using namespace impl_;

			for(auto it = db->find(startKey); it != std::end(*db); ++it){
				auto const &key = it->getKey();

				if (predicate(key))
					break;

				++iterations;

				if (iterations > ITERATIONS_LOOPS_VSIM)
					return key;

				if (!it->isOK())
					continue;


				std::string_view sv = it->getVal();

				const auto *storedVectorB = MyVectors::toStoredVector<T>(sv, dim_ix);

				if (!storedVectorB)
					continue;


				auto const text = shared::extractnth::extractNth(2, DBAdapter::SEPARATOR[0], key);

				if (text == keySub)
					continue;

				float const dist = distancePrepared(dtype,
							vectorPreparedA,	storedVectorB->toVector(),
							magnitudePreparedA,	storedVectorB->magnitude()
				);

				heap.push(VSIMHeapNode{ dist, text });
			}

			return "";
		}

		template<typename T, typename VSIM_Heap>
		static void process_range_finish__(DBAdapter &, Result<Protocol> &result, OutputBlob &blob,
						impl_::DType dtype,
							VSIM_Heap &heap,
								std::string_view tail){

			static_assert(std::is_same_v<VSIM_Heap, VSIMHeap>);

			using namespace impl_;

			auto &container  = blob.construct<OutputBlob::Container>();
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			auto &data = heap.sort();

			for(auto [distPop, text] : data){
				container.push_back(text);

				bcontainer.push_back();

				auto const dist = distanceFix(dtype, distPop);

				auto const val  = formatDouble(dist, bcontainer.back());

				container.push_back(val);
			}

			container.push_back(tail);

			return result.set_container(container);
		}

		template<typename StopPredicate, typename VSIM_Heap>
		static std::string_view process_rangeBit__(DBAdapter &db,
					StopPredicate predicate,
					std::string_view startKey,
					std::string_view /* keyN */, std::string_view keySub,
						uint32_t const dim_ix, impl_::DType dtype,
							MyVectors::BVector vectorA,
								VSIM_Heap &heap, uint32_t &iterations){

			static_assert(std::is_same_v<VSIM_Heap, VSIMHeap>);

			using namespace impl_;

			for(auto it = db->find(startKey); it != std::end(*db); ++it){
				auto const &key = it->getKey();

				if (predicate(key))
					break;

				++iterations;

				if (iterations > ITERATIONS_LOOPS_VSIM)
					return key;

				if (!it->isOK())
					continue;


				std::string_view sv = it->getVal();

				const auto *storedVectorB = MyVectors::toStoredVector<bool>(sv, dim_ix);

				if (!storedVectorB)
					continue;


				auto const text = shared::extractnth::extractNth(2, DBAdapter::SEPARATOR[0], key);

				if (text == keySub)
					continue;

				float const dist = distanceBit(dtype,
								vectorA,
								storedVectorB->toVector()
				);

				heap.push(VSIMHeapNode{ dist, text });
			}

			return "";
		}

		template<typename VSIM_Heap>
		static void process_rangeBit_finish__(DBAdapter &, Result<Protocol> &result, OutputBlob &blob,
						impl_::DType dtype, float fix,
							VSIM_Heap &heap,
								std::string_view tail){

			static_assert(std::is_same_v<VSIM_Heap, VSIMHeap>);

			using namespace impl_;

			auto &container  = blob.construct<OutputBlob::Container>();
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			auto &data = heap.sort();

			for(auto [distPop, text] : data){
				container.push_back(text);

				bcontainer.push_back();

				auto const dist = distanceBitFix(dtype, distPop, fix);

				auto const val  = formatDouble(dist, bcontainer.back());

				container.push_back(val);
			}

			container.push_back(tail);

			return result.set_container(container);
		}

	private:
		// making this same value as VSIM
		constexpr static size_t results__ 		= 32;
		constexpr static size_t count___		= 1000;
		constexpr static size_t ITERATIONS_LOOPS_VSIM	=
						impl_::Decoder<float>::MAX_BANDS * count___;

		using VSIMHeapNode	= std::pair<float, std::string_view>;
		using VSIMHeap		= top_heap::TopKSmallest<VSIMHeapNode, results__>;

	private:
		static_assert(results__						<  OutputBlob::LargeContainerSize);

	private:
		constexpr inline static std::string_view cmd__[] = {
			"vsimflat",	"VSIMFLAT"
		};

	};



	template<class Protocol, class DBAdapter>
	struct VSIM : BaseCommandRO<Protocol,DBAdapter>{

		VSIM() : BaseCommandRO<Protocol,DBAdapter>("VSIM", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// VSIM key   DIM_IX QUANTIZE_TYPE DISTANCE_TYPE name BANDS BITS
		// VSIM words 300    F             C             frog 32    12
		// VSIM words 300    I             C             frog 32    16

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		S = int16
		I = int8
		B = bit

		VEC_TYPE:
		B = binary BE
		b = binary LE
		H = hex BE
		h = hex LE

		DISTANCE_TYPE:
		E = Euclidean L2
		M = Manhattan L1
		C = Cosine
		K = Canberra

		DISTANCE_TYPE for bit vectors:
		H = Hamming
		B = Bit Cosine
		D = Dominate
		*/

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 8)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_7);

			auto const keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const dim_ix = from_string<uint32_t>(p[2]);

			if (dim_ix < 1 || dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[3]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const dtype = translateDType(p[4]);

			if (!checkDType(dtype, qtype))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const keySub = p[5];

			if (keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using MyDecoderF = impl_::Decoder<float>;
			auto const bands = MyDecoderF::fixBands(from_string<uint64_t>(p[6]));
			auto const bits  = MyDecoderF::fixBits (from_string<uint64_t>(p[7]));

			if (dim_ix < 1 || dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			if (!shared::rsetmulti::valid(keyN, keySub, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			switch(qtype){
			case QType::F32	: return process__<float	>(db, result, blob, keyN, keySub, dim_ix, dtype, bands, bits);
			case QType::I16	: return process__<int16_t	>(db, result, blob, keyN, keySub, dim_ix, dtype, bands, bits);
			case QType::I8	: return process__<int8_t	>(db, result, blob, keyN, keySub, dim_ix, dtype, bands, bits);
			case QType::BIT	: return process__<bool		>(db, result, blob, keyN, keySub, dim_ix, dtype, bands, bits);

			default		: return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		template<typename T>
		static void process__(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
						std::string_view keyN, std::string_view keySub,
							uint32_t const dim_ix, impl_::DType dtype,
								size_t bands, size_t bits){
			using namespace impl_;

			std::string_view vectorSV = shared::rsetmulti::getData(db, keyN, keySub);

			auto const pp = prepareFVector<T>(blob, vectorSV, dim_ix, dtype);

			if (!pp.ok)
				return result.set_container0();

			auto const vectorPrepared	= pp.vector;
			auto const magnitudePrepared	= pp.magnitude;


			using MyDecoder  = Decoder<T>;

			static_assert(OutputBlob::Container::capacity() >= MyDecoder::IContainer::capacity());

			auto &icontainer = blob.construct<OutputBlob::Container		>(); // will reuse it later
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer	>(); // will reuse it later

			MyDecoder decoder{ dim_ix, bands, bits };

			bool const b = shared::rsetmulti::getIndexes(decoder, vectorSV,
								icontainer, bcontainer);

			if (!b)
				return result.set_container0();



			auto &keySub_container  = blob.construct<OutputBlob::LargeContainer>();

			for(auto const &index : icontainer){
				hm4::PairBufferKey bufferKey;
				auto const prefix = shared::rsetmulti::makeKeyDataSearchNS(bufferKey, DBAdapter::SEPARATOR, keyN, index);

				scanIndex__(db, prefix, keySub_container);
			}

			// icontainer and bcontainer no longer need.

			std::sort(std::begin(keySub_container), std::end(keySub_container));
			auto uniq_end = std::unique(std::begin(keySub_container), std::end(keySub_container));

			auto &heap = blob.construct<VSIMHeap>();

			logger<Logger::DEBUG>() << "VSIM" << "total candidates" << std::distance(std::begin(keySub_container), uniq_end);

			for(auto it = std::begin(keySub_container); it != uniq_end; ++it){
				auto const &text = *it;

				if (text == keySub)
					continue;

				std::string_view sv = shared::rsetmulti::getData(db, keyN, text);

				const auto *storedVectorB = MyVectors::toStoredVector<T>(sv, dim_ix);

				if (!storedVectorB)
					continue;

				float const dist = distancePrepared(dtype,
							vectorPrepared,		storedVectorB->toVector(),
							magnitudePrepared,	storedVectorB->magnitude()
				);

				heap.push(VSIMHeapNode{ dist, text });
			}

			// keySub_container, icontainer and bcontainer no longer need.

			// reusing icontainer and bcontainer
			auto &container  = icontainer;

			container.clear();
			bcontainer.clear();

			auto &data = heap.sort();

			for(auto [distPop, text] : data){
				container.push_back(text);

				bcontainer.push_back();

				auto const dist = distanceFix(dtype, distPop);

				auto const val  = formatDouble(dist, bcontainer.back());

				container.push_back(val);
			}

			return result.set_container(container);
		}

		template<>
		/* static */ void process__<bool>(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
						std::string_view keyN, std::string_view keySub,
							uint32_t const dim_ix, impl_::DType dtype,
								size_t bands, size_t bits){
			using namespace impl_;

			std::string_view sv = shared::rsetmulti::getData(db, keyN, keySub);

			const auto *storedVectorA = MyVectors::toStoredVector<bool>(sv, dim_ix);

			if (!storedVectorA)
				return result.set_container0();

			auto const vectorA = storedVectorA->toVector();

			using MyDecoder  = Decoder<bool>;

			static_assert(OutputBlob::Container::capacity() >= MyDecoder::IContainer::capacity());

			auto &icontainer = blob.construct<OutputBlob::Container		>(); // will reuse it later
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer	>(); // will reuse it later

			MyDecoder decoder{ dim_ix, bands, bits };

			bool const b = shared::rsetmulti::getIndexes(decoder, sv,
								icontainer, bcontainer);

			if (!b)
				return result.set_container0();



			auto &keySub_container  = blob.construct<OutputBlob::LargeContainer>();

			for(auto const &index : icontainer){
				hm4::PairBufferKey bufferKey;
				auto const prefix = shared::rsetmulti::makeKeyDataSearchNS(bufferKey, DBAdapter::SEPARATOR, keyN, index);

				scanIndex__(db, prefix, keySub_container);
			}

			// icontainer and bcontainer no longer need.

			std::sort(std::begin(keySub_container), std::end(keySub_container));
			auto uniq_end = std::unique(std::begin(keySub_container), std::end(keySub_container));

			auto &heap = blob.construct<VSIMHeap>();

			logger<Logger::DEBUG>() << "VSIM.BIT" << "total candidates" << std::distance(std::begin(keySub_container), uniq_end);

			auto const fix = distanceBitPrepareFix(dtype, vectorA);

			for(auto it = std::begin(keySub_container); it != uniq_end; ++it){
				auto const &text = *it;

				if (text == keySub)
					continue;

				std::string_view sv = shared::rsetmulti::getData(db, keyN, text);

				const auto *storedVectorB = MyVectors::toStoredVector<bool>(sv, dim_ix);

				if (!storedVectorB)
					continue;

				float const dist = distanceBit(dtype,
								vectorA,
								storedVectorB->toVector()
				);

				heap.push(VSIMHeapNode{ dist, text });
			}

			// keySub_container, icontainer and bcontainer no longer need.

			// reusing icontainer and bcontainer
			auto &container  = icontainer;

			container.clear();
			bcontainer.clear();

			auto &data = heap.sort();

			for(auto [distPop, text] : data){
				container.push_back(text);

				bcontainer.push_back();

				auto const dist = distanceBitFix(dtype, distPop, fix);

				auto const val  = formatDouble(dist, bcontainer.back());

				container.push_back(val);
			}

			return result.set_container(container);
		}

		static void scanIndex__(DBAdapter &db, std::string_view prefix, OutputBlob::LargeContainer &container){
			using namespace shared::accumulate_results;

			auto const &key = prefix;

			logger<Logger::DEBUG>() << "VSIM" << "prefix" << prefix << "key" << key;

			StopPrefixPredicate stop{ prefix };

			auto proj = [](std::string_view x){
				[[maybe_unused]]
				auto const separator = DBAdapter::SEPARATOR[0];

				// keyN~word~keySort~keySub
				return shared::extractnth::extractNth(3, separator, x);
			};

			auto const Out = AccumulateOutput::KEYS;

			sharedAccumulateResults<Out>(
				count__		,
				stop		,
				db->find(key)	,
				std::end(*db)	,
				container	,
				proj
			);

			logger<Logger::DEBUG>() << "VSIM" << "prefix" << container.size() << "candidates";
		}

	private:
		constexpr static size_t results__	= 32;
		constexpr static size_t count__		= 1000;

		using VSIMHeapNode	= std::pair<float, std::string_view>;
		using VSIMHeap		= top_heap::TopKSmallest<VSIMHeapNode, results__>;

	private:
		static_assert(results__						<  OutputBlob::LargeContainerSize);
		static_assert(impl_::Decoder<float>::MAX_BANDS * count__	<  OutputBlob::LargeContainerSize);

	private:
		constexpr inline static std::string_view cmd__[] = {
			"vsim",		"VSIM"
		};

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "vectors";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				VADD		,
				VREM		,
				VGETINDEXES	,
				VGET		,
				VGETNORMALIZED	,
				VGETRAW		,
				VDISTANCE	,
				VMDISTANCE	,
				VSIMFLAT	,
				VSIM
			>(pack);
		}
	};



} // namespace




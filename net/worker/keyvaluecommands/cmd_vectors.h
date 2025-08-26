#include "base.h"

#include "logger.h"

#include "fmt/format.h"

#include "shared_zset_multi.h"
#include "shared_iterations.h"
#include "shared_stoppredicate.h"

#include "ilist/txguard.h"

#include "mystring.h"
#include "hexconvert.h"
#include "myhamming.h"
#include "vectors.h"
#include "bitvectors.h"

#include "mybufferview.h"

#include "myendian.h"

#include "logger.h"



namespace net::worker::commands::Vectors{

	using FVector  = MyBuffer::BufferView<float>;

	template<typename T>
	using CVector  = MyBuffer::BufferView<T const>;

	using CFVector = CVector<float>;



	namespace vectors_impl_{

		inline std::string_view extractNth_(size_t const nth, char const separator, std::string_view const s){
			size_t count = 0;

			for (size_t i = 0; i < s.size(); ++i)
				if (s[i] == separator)
					if (++count; count == nth)
						return s.substr(i + 1);

			return "INVALID_DATA";
		}

		using P1 = net::worker::shared::zsetmulti::Permutation1NoIndex;

		constexpr bool isVectorsKeyValid(std::string_view key, std::string_view name){
			return P1::valid(key, name, 4);
		}

		template<size_t N>
		std::string_view formatV(double d, std::array<char, N> &buffer){
			constexpr static std::string_view fmt_mask = "{:+015.15f}";

			auto const result = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask, d);

			if (result.out == std::end(buffer))
				return {};
			else
				return { buffer.data(), result.size };
		};

		template<size_t N>
		std::string_view formatV(int8_t d, std::array<char, N> &buffer){
			return to_string(d, buffer);
		};



		// renamed to Wector in order to be distinguished from MyVectors::*Vector*

		template<typename T>
		struct Wector{
			static_assert(MyVectors::checkVectorElement<T>(), "Only float, int8_t and int16_t supported");

			float		magnitudeBE;	// 4
			uint32_t	dimBE;		// 4
			T		vdata[1];	// flexible member

			constexpr static const Wector *createInRawMemory(void *mem, CFVector const vector){
				using namespace MyVectors;

				auto *self = static_cast<Wector *>(mem);

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

			constexpr auto magnitude() const{
				return betoh(magnitudeBE);
			}

			constexpr std::string_view toSV() const{
				return std::string_view{
					reinterpret_cast<const char *>(this),
					bytes()
				};
			}

			constexpr CVector<T> toVector() const{
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

				return { vdata, dim() };

				#pragma GCC diagnostic pop
			}

			constexpr size_t bytes() const{
				return bytes(dim());
			}

			constexpr static size_t bytes(size_t dim){
				return sizeof(Wector) - sizeof(T) + dim * sizeof(T);
			}
		} __attribute__((__packed__));

		static_assert(std::is_standard_layout_v<Wector<float	> >, "Wector must be POD type");
		static_assert(std::is_standard_layout_v<Wector<int16_t	> >, "Wector must be POD type");
		static_assert(std::is_standard_layout_v<Wector<int8_t	> >, "Wector must be POD type");



		template<>
		struct Wector<bool>{
			uint32_t	dimBE;		// 4
			uint8_t		vdata[1];	// flexible member

			static const Wector *createInRawMemory(void *mem, CFVector const vector){
				using namespace MyVectors;

				auto *self = static_cast<Wector *>(mem);

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
		};

		static_assert(std::is_standard_layout_v<Wector<bool	> >, "Wector must be POD type");



		constexpr uint32_t MaxDimensions = 1024 * 8;



		using FVectorBuffer = std::array<char, MaxDimensions * sizeof(float)>;	// 32 KB



		template<typename T>
		using WectorBuffer = std::array<char, Wector<T>::bytes(MaxDimensions)>; // VectorBuffer + 8



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



		enum class DType{
			UNKNOWN		,
			EUCLIDEAN	,
			MANHATTAN	,
			COSINE		,
			CANBERRA	,
			HAMMING
		};

		constexpr auto translateDType(std::string_view s){
			if (s.size() != 1)
				return DType::UNKNOWN;

			switch(s[0]){
			case 'l' :
			case 'L' :
			case 'e' :
			case 'E' :	return DType::EUCLIDEAN	;

			case 'm' :
			case 'M' :	return DType::MANHATTAN	;

			case 'c' :
			case 'C' :	return DType::COSINE	;

			case 'k' :
			case 'K' :	return DType::CANBERRA	;

			case 'h' :
			case 'H' :	return DType::HAMMING	;

			default:	return DType::UNKNOWN	;
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



		namespace {

			void prepareHash(uint8_t *hashOut, FVector fvectorResult){
				// step 3 - get hash by projecting it again

				if (hashOut)
					*hashOut = MyVectors::randomProjectionBit<uint8_t>(fvectorResult, MaxDimensions);
			}

			FVector prepareFVector_project(uint8_t *hashOut, FVector fvectorOriginal, FVector fvectorResult){
				// step 2.2 - project

				MyVectors::randomProjection(fvectorOriginal, fvectorResult);

				// we have float vector in final format, without normalization, in correct buffer...

				// step 3 - get hash by projecting it again

				prepareHash(hashOut, fvectorResult);

				// step 4 - done

				return fvectorResult;
			}

			FVector prepareFVector_direct(uint8_t *hashOut, FVector fvectorOriginal, FVector fvectorResult){
				// step 2.2 - non optimized copy

				const auto &in  = fvectorOriginal;
				      auto &out = fvectorResult;

				for(size_t i = 0; i < fvectorOriginal.size(); ++i)
					out[i] = in[i];

				// we have float vector in final format, without normalization, in correct buffer...

				// step 3 - get hash by projecting it again

				prepareHash(hashOut, fvectorResult);

				// step 4 - done

				return fvectorResult;
			}

			FVector prepareFVector(uint8_t *hashOut, VType vtype, std::string_view fvectorSV, uint32_t const dim_ve, uint32_t const dim_ix, FVectorBuffer &fVectorBufferResult, OutputBlob &blob){
				// step 1 - convert vector into float[]

				bool const needsToBeProjected = dim_ix > 1 && dim_ix < dim_ve;

				auto &fVectorBufferConvert = blob.allocate<FVectorBuffer>();

				FVector fvectorOriginal = [vtype, dim_ve](std::string_view fvectorSV, FVectorBuffer &buffer){
					switch(vtype){
					default:
					case VType::BINARY_LE :
					case VType::BINARY_BE : {

							// we have to memcpy() because we need to be able to return mutable version,
							// but fvectorSV is const char *

							// vectorSV size is checked already

							const float *in  = reinterpret_cast<const float *>(fvectorSV.data()	);
							      float *out = reinterpret_cast<      float *>(buffer.data()	);

							for(size_t i = 0; i < dim_ve; ++i)
								out[i] = in[i];

							FVector result{
								out,
								dim_ve
							};

							assert(result.size() == dim_ve);

							return result;
						}

					case VType::HEX_LE :
					case VType::HEX_BE : {
							// vectorSV size is checked already

							hex_convert::fromHexBytes(fvectorSV, buffer);

							FVector result{
								reinterpret_cast<float *>(buffer.data()),
								dim_ve
							};

							assert(result.size() == dim_ve);

							return result;
						}
					}
				}(fvectorSV, fVectorBufferConvert);

				// fvectorOriginal is now decoded to float[],
				// it live in bufferConvert,
				// depends if it will be projected in a moment.

				// step 2 - project the vector if needed and create a hash, if needed

				// step 2.0 - convert vector host order, else mutations will not work.

				switch(vtype){
				default:
				case VType::BINARY_LE :
				case VType::HEX_LE    : {

						#pragma GCC ivdep
						for(size_t i = 0; i < fvectorOriginal.size(); ++i)
							fvectorOriginal[i] = letoh(fvectorOriginal[i]);

						break;
					}

				case VType::BINARY_BE :
				case VType::HEX_BE    : {

						#pragma GCC ivdep
						for(size_t i = 0; i < fvectorOriginal.size(); ++i)
							fvectorOriginal[i] = betoh(fvectorOriginal[i]);

						break;
					}
				}

				// step 2.1 - create result vector

				FVector fvectorResult{ fVectorBufferResult.data(), dim_ix * sizeof(float) };

				if (needsToBeProjected){
					return prepareFVector_project(hashOut, fvectorOriginal, fvectorResult);
				}else{
					return prepareFVector_direct (hashOut, fvectorOriginal, fvectorResult);
				}
			}

			FVector prepareFVector(VType vtype, std::string_view fvectorSV, uint32_t const dim_ve, uint32_t const dim_ix, FVectorBuffer &fVectorBufferResult, OutputBlob &blob){
				return prepareFVector(nullptr, vtype, fvectorSV, dim_ve, dim_ix, fVectorBufferResult, blob);
			}

			template<typename T>
			const Wector<T> *prepareWectorBE(uint8_t *hashOut, VType vtype, std::string_view fvectorSV, uint32_t const dim_ve, uint32_t const dim_ix, WectorBuffer<T> &wectorBufferResult, OutputBlob &blob){
				auto &fVectorBuffer = blob.allocate<FVectorBuffer>();

				FVector vector = prepareFVector(hashOut, vtype, fvectorSV, dim_ve, dim_ix, fVectorBuffer, blob);

				void *mem = wectorBufferResult.data();

				return Wector<T>::createInRawMemory(mem, vector);
			}

			template<typename T>
			const Wector<T> *prepareWectorBE(VType vtype, std::string_view fvectorSV, uint32_t const dim_ve, uint32_t const dim_ix, WectorBuffer<T> &wectorBufferResult, OutputBlob &blob){
				return prepareWectorBE<T>(nullptr, vtype, fvectorSV, dim_ve, dim_ix, wectorBufferResult, blob);
			}

		} // anonymous namespace



		template<typename T, bool Norm, typename Protocol>
		void process_VGET_(Result<Protocol> &result, OutputBlob &blob, std::string_view const wectorSV, uint32_t const dim_ix){
			using namespace MyVectors;

			if (wectorSV.size() != Wector<T>::bytes(dim_ix))
				return result.set(false);

			const auto *bv = reinterpret_cast<const Wector<T> *>(wectorSV.data());

			if (dim_ix != bv->dim())
				return result.set(false);

			if constexpr(!std::is_same_v<T, bool>){
				auto const vector	= bv->toVector();
				auto const magnitude	= bv->magnitude();

				auto &container  = blob.construct<OutputBlob::Container>();
				auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

				if constexpr(Norm){
					bcontainer.push_back();

					auto const s = formatV(magnitude, bcontainer.back());

					container.push_back(s);
				}

				auto f = [&container, &bcontainer](size_t, float const value){
					bcontainer.push_back();

					auto const s = formatV(value, bcontainer.back());

					container.push_back(s);
				};

				dequantizeF(vector, f, valueProjBE);

				return result.set_container(container);
			}else{
				auto &container  = blob.construct<OutputBlob::Container>();

				if constexpr(Norm){
					container.push_back("0");
				}

				for (size_t i = 0; i < dim_ix; ++i){
					auto const s = bv->operator [](i) ? "+1" : "-1";

					container.push_back(s);
				}

				return result.set_container(container);
			}
		}

		template<typename T, typename Protocol>
		void process_VGET(Result<Protocol> &result, OutputBlob &blob, std::string_view const wectorSV, uint32_t const dim_ix){
			return process_VGET_<T, 0>(result, blob, wectorSV, dim_ix);
		}

		template<typename T, typename Protocol>
		void process_VGETNORMALIZED(Result<Protocol> &result, OutputBlob &blob, std::string_view const wectorSV, uint32_t const dim_ix){
			return process_VGET_<T, 1>(result, blob, wectorSV, dim_ix);
		}

		template<typename T, typename Protocol>
		void process_VGETRAW(Result<Protocol> &result, OutputBlob &blob, std::string_view const wectorSV, uint32_t const dim_ix, vectors_impl_::VType vtype){
			using namespace MyVectors;

			if (wectorSV.size() != Wector<T>::bytes(dim_ix))
				return result.set(false);

			const auto *bv = reinterpret_cast<const Wector<T> *>(wectorSV.data());

			if (dim_ix != bv->dim())
				return result.set(false);

			switch(vtype){
			default:
			case VType::BINARY_LE :
			case VType::BINARY_BE : {
					auto &fVectorBufferResult = blob.allocate<FVectorBuffer>();

					FVector fvector{ fVectorBufferResult };

					if constexpr(!std::is_same_v<T, bool>){
						auto const vector	= bv->toVector();
						auto const magnitude	= bv->magnitude();

						auto f = [&fvector](size_t i, float const value){
							fvector[i] = value;
						};

						denormalizeF(vector, magnitude, f, valueProjBE);

						// fvector is denormalized and in machine order now.

						if (vtype == VType::BINARY_BE){
							#pragma GCC ivdep
							for(size_t i = 0; i < fvector.size(); ++i)
								fvector[i] = htobe(fvector[i]);
						}else{
							#pragma GCC ivdep
							for(size_t i = 0; i < fvector.size(); ++i)
								fvector[i] = htole(fvector[i]);
						}
					}else{
						float const fp = vtype == VType::BINARY_BE ? htobe(+1.f) : htole(+1.f);
						float const fn = vtype == VType::BINARY_BE ? htobe(-1.f) : htole(-1.f);

						for (size_t i = 0; i < dim_ix; ++i)
							fvector[i] = bv->operator [](i) ? fp : fn;
					}

					return result.set(
						std::string_view{
							fVectorBufferResult.data(),
							dim_ix * sizeof(float)
						}
					);
				}

			case VType::HEX_LE :
			case VType::HEX_BE : {
					auto const sizeF = MaxDimensions * sizeof(float) * 2;
					std::array<char, sizeF> buffer;

					// however our toHex() is big endian, so we have to negate

					if (vtype != VType::HEX_BE){
						// big endian data

						auto f_be = [&buffer](size_t i, float const value_){
							float const value = htobe(value_);

							uint32_t const u32 = bit_cast<uint32_t>(value);

							char *buff = buffer.data() + i * sizeof(float) * 2;

							hex_convert::toHex(u32, buff);
						};

						if constexpr(!std::is_same_v<T, bool>){
							auto const vector	= bv->toVector();
							auto const magnitude	= bv->magnitude();

							denormalizeF(vector, magnitude, f_be, valueProjBE);
						}else{
							float const fp = +1.f;
							float const fn = -1.f;

							for (size_t i = 0; i < dim_ix; ++i)
								f_be(i, bv->operator [](i) ? fp : fn);
						}
					}else{
						// little endian data

						auto f_le = [&buffer](size_t i, float const value_){
							float const value = htole(value_);

							uint32_t const u32 = bit_cast<uint32_t>(value);

							char *buff = buffer.data() + i * sizeof(float) * 2;

							hex_convert::toHex(u32, buff);
						};

						if constexpr(!std::is_same_v<T, bool>){
							auto const vector	= bv->toVector();
							auto const magnitude	= bv->magnitude();

							denormalizeF(vector, magnitude, f_le, valueProjBE);
						}else{
							float const fp = +1.f;
							float const fn = -1.f;

							for (size_t i = 0; i < dim_ix; ++i)
								f_le(i, bv->operator [](i) ? fp : fn);
						}
					}

					return result.set(
						std::string_view{
							buffer.data(),
							dim_ix * sizeof(float) * 2
						}
					);
				}
			} // switch vtype
		}



		template<typename T>
		struct VSIMHeapData{
			std::string_view	key;
			T			score;

			constexpr VSIMHeapData(std::string_view key, T score) : key(key), score(score){}

			constexpr bool operator<(VSIMHeapData const &other) const{
				return score < other.score;
			}
		};

		constexpr static size_t VSIM_MAX_RESULTS = 5000;

		// 118KB, if sv is 16 bytes, same for bits
		template<typename T>
		using VSIMHeap     = StaticVector<VSIMHeapData<T>, VSIM_MAX_RESULTS + 1>;

		using VSIMHeapFloat = VSIMHeap<float >;
		using VSIMHeapSize  = VSIMHeap<size_t>;

		constexpr uint32_t ITERATIONS_LOOPS_VSIM = 1'000'000;



		template<typename T>
		void insertIntoHeap(uint32_t const results, VSIMHeap<T> &heap, std::string_view const key, T const score){
			if (heap.size() >= results && score >= heap.front().score)
				return;

			heap.emplace_back(key, score);
			std::push_heap(std::begin(heap), std::end(heap));

			if (heap.size() > results){
				std::pop_heap(std::begin(heap), std::end(heap));
				heap.pop_back();
			}
		}

		template<typename T, typename DBAdapter, typename StopPredicate>
		void process_VSIM_range(DBAdapter &db,
					StopPredicate predicate,
					std::string_view prefix,
					uint32_t const dim_ix,
					DType dtype,
					CFVector const original_fvector, float const original_magnitude,
					uint32_t const results,
					VSIMHeapFloat &heap, uint32_t &iterations){

			for(auto it = db->find(prefix); it != std::end(*db); ++it){
				auto const &key = it->getKey();

				if (predicate(key))
					break;

				++iterations;

				if (iterations > ITERATIONS_LOOPS_VSIM)
					break;

				if (!it->isOK())
					continue;

				auto const vblob = it->getVal();

				// invalid size, skip this one.
				if (vblob.size() != Wector<T>::bytes(dim_ix))
					continue;

				const auto *bv = reinterpret_cast<const Wector<T> *>(vblob.data());

				// SEARCH "IN"-CONDITION
				auto const score = [&](auto const &a, auto const &b, auto const &aM, auto const &bM){
					switch(dtype){
					default:
					case DType::HAMMING	:
					case DType::COSINE	: return MyVectors::distanceCosine		(a, b,         {}, valueProjBE);
					case DType::EUCLIDEAN	: return MyVectors::distanceEuclideanSquared	(a, b, aM, bM, {}, valueProjBE);
					case DType::MANHATTAN	: return MyVectors::distanceManhattanPrepared	(a, b,     bM, {}, valueProjBE);
					case DType::CANBERRA	: return MyVectors::distanceCanberraPrepared	(a, b,     bM, {}, valueProjBE);
					}
				}(original_fvector, bv->toVector(), original_magnitude, bv->magnitude());

				insertIntoHeap(results, heap, key, score);
			}
		}

		template<typename DBAdapter, typename StopPredicate>
		void process_VSIM_rangeBit(DBAdapter &db,
					StopPredicate predicate,
					std::string_view prefix,
					uint32_t const dim_ix,
					std::string_view const original_bitVector,
					uint32_t const results,
					VSIMHeapSize &heap, uint32_t &iterations){

			for(auto it = db->find(prefix); it != std::end(*db); ++it){
				auto const &key = it->getKey();

				if (predicate(key))
					break;

				++iterations;

				if (iterations > ITERATIONS_LOOPS_VSIM)
					break;

				if (!it->isOK())
					continue;

				auto const vblob = it->getVal();

				// invalid size, skip this one.
				if (vblob.size() != Wector<bool>::bytes(dim_ix))
					continue;

				const auto *bv = reinterpret_cast<const Wector<bool> *>(vblob.data());

				// SEARCH "IN"-CONDITION

				auto const score = MyVectors::distanceHamming(original_bitVector, bv->toBitSV());

				insertIntoHeap(results, heap, key, score);
			}
		}

		constexpr float process_VSIM_prepareFVector(DType dtype, FVector &fvector){
			// SEARCH PRE-CONDITION
			switch(dtype){
			default:
			case DType::EUCLIDEAN	:
			case DType::COSINE	: return MyVectors::normalizeInline(fvector);

			case DType::HAMMING	:
			case DType::CANBERRA	:
			case DType::MANHATTAN	: return 1.f; /* skip normalization */
			}
		}

		template<bool FullKey = false, typename Protocol, typename DBAdapter, typename T>
		void process_VSIM_finish(DBAdapter &, Result<Protocol> &result, OutputBlob &blob,
					DType dtype, VSIMHeap<T> &heap, CFVector const original_fvector){

			(void) original_fvector;

			// usually we give results unsorted,
			// but here they are almost heap-sorted...
			std::sort_heap(std::begin(heap), std::end(heap));

			auto &container  = blob.construct<OutputBlob::Container>();
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			auto const hammingFix = 1 / static_cast<float>(original_fvector.size());

			for(auto it = std::begin(heap); it != std::end(heap); ++it){

				if constexpr(FullKey){
					auto const key = it->key;

					container.push_back(key);
				}else{
					auto const key = extractNth_(2, DBAdapter::SEPARATOR[0], it->key);

					container.push_back(key);
				}

				bcontainer.push_back();

				// SEARCH POST-CONDITION
				auto const score = [&](auto score) -> float{
					if constexpr(std::is_same_v<T, float>){
						switch(dtype){
						case DType::COSINE	:
						case DType::MANHATTAN	:
						case DType::CANBERRA	: return score;
						case DType::EUCLIDEAN	: return std::sqrt(score);

						default			: return 1; // will never come here.
						}
					}else{
						switch(dtype){
						case DType::HAMMING	: return static_cast<float>(score) * hammingFix;

						default			: return 1; // will never come here.
						}
					}
				}(it->score);

				auto const val = formatV(score, bcontainer.back());

				container.push_back(val);
			}

			return result.set_container(container);
		}

	} // namespace vectors_impl_



	template<class Protocol, class DBAdapter>
	struct VADD : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// VADD key   DIM_VE DIM_IX QUANTIZE_TYPE VEC_TYPE BLOB  name BLOB  name ...
		// VADD words 300    150    F             b        BLOB0 frog BLOB1 cat
		// VADD words 300    150    I             b        BLOB0 frog BLOB1 cat

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		I = int8

		VEC_TYPE:
		B = binary BE
		b = binary LE
		H = hex BE
		h = hex LE
		*/

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace vectors_impl_;

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

			if (dim_ix <= 1 || dim_ix > dim_ve)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[4]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const vtype = translateVType(p[5]);

			if (vtype == VType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			switch(qtype){
			case QType::F32	: return process_<float		>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, dim_ve, dim_ix, vtype);
			case QType::I16	: return process_<int16_t	>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, dim_ve, dim_ix, vtype);
			case QType::I8	: return process_<int8_t	>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, dim_ve, dim_ix, vtype);
			case QType::BIT	: return process_<bool		>(std::begin(p) + varg, std::end(p), db, result, blob, keyN, dim_ve, dim_ix, vtype);

			default		: return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

	private:
		template<typename T, typename IT>
		static void process_(IT first, IT last, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob, std::string_view keyN, uint32_t const dim_ve, uint32_t const dim_ix, vectors_impl_::VType vtype){
			using namespace vectors_impl_;

			auto const vstep = 2;

			auto const sizeM = translateVTypeToSizeM(vtype);

			for(auto itk = first; itk != last; itk += vstep){
				auto const vectorSV = *(itk + 0);

				if (!MyVectors::validBlobSizeF(vectorSV.size(), dim_ve * sizeM))
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

				auto const name = *(itk + 1);

				if (name.empty())
					return result.set_error(ResultErrorMessages::EMPTY_NAME);

				if (!isVectorsKeyValid(keyN, name))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}



			auto &wectorBuffer = blob.allocate<WectorBuffer<T> >();



			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			for(auto itk = first; itk != last; itk += vstep){
				auto const vectorSV = *(itk + 0);
				auto const name     = *(itk + 1);

				// vectorSV size is checked already

				uint8_t hashOut;

				const Wector<T> *w = prepareWectorBE<T>(&hashOut, vtype,  vectorSV, dim_ve, dim_ix, wectorBuffer, blob);

				auto const line = w->toSV();

				char buffer[5]; // 00FF\0
				// add 00 in front of the hash
				std::string_view const hash = hex_convert::toHex<uint16_t>(hashOut, buffer);

				shared::zsetmulti::add<P1>(db, keyN, name, { hash }, line);
			}

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"vadd",	"VADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VREM : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace vectors_impl_;

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			return shared::zsetmulti::cmdProcessRem<P1>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"vrem",		"VREM",
			"vremove",	"VREMOVE",
			"vdel",		"VDEL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// VGET key   DIM_IX QUANTIZE_TYPE name
		// VGET words 300    F             frog
		// VGET words 300    I             frog

		/*
		QUANTIZE_TYPE
		F = float	e.g. do not quantize
		I = int8
		*/

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace vectors_impl_;

			auto const &name = p[4];

			if (name.empty())
				return result.set_error(ResultErrorMessages::EMPTY_NAME);

			if (!isVectorsKeyValid(keyN, name))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const dim_ix = from_string<uint32_t>(p[2]);

			if (dim_ix < 1 && dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[3]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const wectorBlob = shared::zsetmulti::get<P1>(db, keyN, name);

			switch(qtype){
			default:
			case QType::F32 : return process_VGET<float	>(result, blob, wectorBlob, dim_ix);
			case QType::I16 : return process_VGET<int16_t	>(result, blob, wectorBlob, dim_ix);
			case QType::I8  : return process_VGET<int8_t	>(result, blob, wectorBlob, dim_ix);
			case QType::BIT : return process_VGET<bool	>(result, blob, wectorBlob, dim_ix);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"vget",		"VGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VGETRAW : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// VGETHEX key   DIM_IX QUANTIZE_TYPE OUT_VEC_TYPE name
		// VGETHEX words 300    F             b            frog
		// VGETHEX words 300    I             b            frog

		/*
		QUANTIZE_TYPE
		F = float	e.g. do not quantize
		I = int8

		VEC_TYPE:
		B = binary BE
		b = binary LE
		H = hex BE
		h = hex LE
		*/

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 6)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace vectors_impl_;

			auto const &name = p[5];

			if (name.empty())
				return result.set_error(ResultErrorMessages::EMPTY_NAME);

			if (!isVectorsKeyValid(keyN, name))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const dim_ix = from_string<uint32_t>(p[2]);

			if (dim_ix < 1 && dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[3]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const vtype = translateVType(p[4]);

			if (vtype == VType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const wectorBlob = shared::zsetmulti::get<P1>(db, keyN, name);

			switch(qtype){
			default:
			case QType::F32 : return process_VGETRAW<float		>(result, blob, wectorBlob, dim_ix, vtype);
			case QType::I16 : return process_VGETRAW<int16_t	>(result, blob, wectorBlob, dim_ix, vtype);
			case QType::I8  : return process_VGETRAW<int8_t		>(result, blob, wectorBlob, dim_ix, vtype);
			case QType::BIT : return process_VGETRAW<bool		>(result, blob, wectorBlob, dim_ix, vtype);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"vgetraw",	"VGETRAW",
			"vgetblob",	"VGETBLOB"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VGETNORMALIZED : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// VGETNORMALIZED key   DIM_IX QUANTIZE_TYPE name
		// VGETNORMALIZED words 300    F             frog
		// VGETNORMALIZED words 300    I             frog

		/*
		QUANTIZE_TYPE
		F = float	e.g. do not quantize
		I = int8
		*/

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace vectors_impl_;

			auto const &name = p[4];

			if (name.empty())
				return result.set_error(ResultErrorMessages::EMPTY_NAME);

			if (!isVectorsKeyValid(keyN, name))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const dim_ix = from_string<uint32_t>(p[2]);

			if (dim_ix < 1 && dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[3]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const wectorBlob = shared::zsetmulti::get<P1>(db, keyN, name);

			switch(qtype){
			default:
			case QType::F32 : return process_VGETNORMALIZED<float	>(result, blob, wectorBlob, dim_ix);
			case QType::I16 : return process_VGETNORMALIZED<int16_t	>(result, blob, wectorBlob, dim_ix);
			case QType::I8  : return process_VGETNORMALIZED<int8_t	>(result, blob, wectorBlob, dim_ix);
			case QType::BIT : return process_VGETNORMALIZED<bool	>(result, blob, wectorBlob, dim_ix);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"vgetnormalized",	"VGETNORMALIZED",
			"vgetnorm",		"VGETNORM"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VSIMFLAT : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// VSIMFLAT key   DIM_VE DIM_IX QUANTIZE_TYPE DISTANCE_TYPE VEC_TYPE BLOB RESULTS
		// VSIMFLAT words 300    300    F             C             b        BLOB 100
		// VSIMFLAT words 300    300    I             C             b        BLOB 100

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		I = int8

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
		*/

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace vectors_impl_;

			if (p.size() != 9)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_8);

			auto const keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const dim_ve = from_string<uint32_t>(p[2]);

			if (dim_ve <  1 || dim_ve > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const dim_ix = from_string<uint32_t>(p[3]);

			if (dim_ix <= 1 || dim_ix > dim_ve)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[4]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const dtype = translateDType(p[5]);

			if (qtype == QType::BIT && dtype != DType::HAMMING)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			else
			if (dtype == DType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const vtype = translateVType(p[6]);

			if (vtype == VType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const sizeM = translateVTypeToSizeM(vtype);

			auto const vectorSV = p[7];

			if (!MyVectors::validBlobSizeF(vectorSV.size(), dim_ve * sizeM))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto &fVectorBuffer = blob.allocate<FVectorBuffer>();

			// vectorSV size is checked already

			/* mutable */ FVector fvector = prepareFVector(vtype, vectorSV, dim_ve, dim_ix, fVectorBuffer, blob);

			auto const magnitude = process_VSIM_prepareFVector(dtype, fvector);

			auto const results = std::clamp<uint32_t>(from_string<uint32_t>(p[8]), 1, VSIM_MAX_RESULTS);

			switch(qtype){
			case QType::F32	: return process_<float		>(db, result, blob, keyN, dim_ix, dtype, fvector, magnitude, results);
			case QType::I16	: return process_<int16_t	>(db, result, blob, keyN, dim_ix, dtype, fvector, magnitude, results);
			case QType::I8	: return process_<int8_t	>(db, result, blob, keyN, dim_ix, dtype, fvector, magnitude, results);
			case QType::BIT	: return process_bit_		 (db, result, blob, keyN, dim_ix,        fvector,            results);

			default		: return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

	private:
		template<typename T>
		static void process_(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
				std::string_view keyN, uint32_t const dim_ix,
				vectors_impl_::DType dtype,
				CFVector const original_fvector, float const original_magnitude,
				uint32_t const results ){

			using namespace vectors_impl_;

			auto &heap = blob.construct<VSIMHeapFloat>();
			// heap_.clear();
			// std::make_heap(std::begin(heap_), std::end(heap_));

			std::string_view const rangeHash = "00";

			hm4::PairBufferKey bufferKey;

			auto const prefix = P1::template makeKey<0>(bufferKey, DBAdapter::SEPARATOR, keyN, /* unused */ std::string_view{}, rangeHash);

			logger<Logger::DEBUG>() << "VSIM.FLAT" << "range" << rangeHash[0] << rangeHash[1];

			shared::stop_predicate::StopPrefixPredicate stop{ prefix };

			uint32_t iterations = 0;

			process_VSIM_range<T>(	db,
						stop,
						prefix,
						dim_ix,
						dtype,
						original_fvector, original_magnitude,
						results,
						heap,
						iterations
			);

			logger<Logger::DEBUG>() << "VSIM.FLAT" << "range finish" << iterations << "vectors";

			return process_VSIM_finish(db, result, blob, dtype, heap, original_fvector);
		}

		static void process_bit_(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
				std::string_view keyN, uint32_t const dim_ix,
				CFVector const original_fvector,
				uint32_t const results ){

			using namespace vectors_impl_;

			auto const bitVectorBytesMax = Wector<bool>::bytes(MaxDimensions);

			std::array<uint8_t, bitVectorBytesMax> bitVectorBuffer;

			MyVectors::bitVectorQuantize(original_fvector, bitVectorBuffer.data());

			auto const bitVector = std::string_view{
					reinterpret_cast<const char *>(bitVectorBuffer.data()),
					MyVectors::bitVectorBytes(original_fvector.size())
			};

			auto &heap = blob.construct<VSIMHeapSize>();
			// heap.clear();
			// std::make_heap(std::begin(heap_), std::end(heap_));

			std::string_view const rangeHash = "00";

			hm4::PairBufferKey bufferKey;

			auto const prefix = P1::template makeKey<0>(bufferKey, DBAdapter::SEPARATOR, keyN, /* unused */ std::string_view{}, rangeHash);

			logger<Logger::DEBUG>() << "VSIM.FLAT.BIT" << "range" << rangeHash[0] << rangeHash[1];

			shared::stop_predicate::StopPrefixPredicate stop{ prefix };

			uint32_t iterations = 0;

			process_VSIM_rangeBit(	db,
						stop,
						prefix,
						dim_ix,
						bitVector,
						results,
						heap,
						iterations
			);

			logger<Logger::DEBUG>() << "VSIM.FLAT.BIT" << "range finish" << iterations << "vectors";

			return process_VSIM_finish(db, result, blob, DType::HAMMING, heap, original_fvector);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"vsimflat",	"VSIMFLAT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VSIMLSH : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// VSIMFLAT key   DIM_VE DIM_IX QUANTIZE_TYPE DISTANCE_TYPE VEC_TYPE BLOB RESULTS
		// VSIMFLAT words 300    300    F             C             b        BLOB 100
		// VSIMFLAT words 300    300    I             C             b        BLOB 100

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		I = int8

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
		*/

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace vectors_impl_;

			if (p.size() != 9)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_8);

			auto const keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const dim_ve = from_string<uint32_t>(p[2]);

			if (dim_ve <  1 || dim_ve > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const dim_ix = from_string<uint32_t>(p[3]);

			if (dim_ix <= 1 || dim_ix > dim_ve)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[4]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const dtype = translateDType(p[5]);

			if (qtype == QType::BIT && dtype != DType::HAMMING)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			else
			if (dtype == DType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const vtype = translateVType(p[6]);

			if (vtype == VType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const sizeM = translateVTypeToSizeM(vtype);

			auto const vectorSV = p[7];

			if (!MyVectors::validBlobSizeF(vectorSV.size(), dim_ve * sizeM))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto &fVectorBuffer = blob.allocate<FVectorBuffer>();

			// vectorSV size is checked already

			uint8_t hashOut;

			/* mutable */ FVector fvector = prepareFVector(&hashOut, vtype, vectorSV, dim_ve, dim_ix, fVectorBuffer, blob);

			auto const magnitude = process_VSIM_prepareFVector(dtype, fvector);

			auto const results = std::clamp<uint32_t>(from_string<uint32_t>(p[8]), 1, VSIM_MAX_RESULTS);

			switch(qtype){
			case QType::F32	: return process_<float		>(db, result, blob, keyN, dim_ix, dtype, fvector, magnitude, hashOut, results);
			case QType::I16	: return process_<int16_t	>(db, result, blob, keyN, dim_ix, dtype, fvector, magnitude, hashOut, results);
			case QType::I8	: return process_<int8_t	>(db, result, blob, keyN, dim_ix, dtype, fvector, magnitude, hashOut, results);
			case QType::BIT	: return process_bit_		 (db, result, blob, keyN, dim_ix,        fvector,            hashOut, results);

			default		: return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

	private:
		template<typename T>
		static void process_(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
				std::string_view keyN, uint32_t const dim_ix,
				vectors_impl_::DType dtype,
				CFVector const original_fvector, float const original_magnitude, uint8_t const lsh,
				uint32_t const results ){

			using namespace vectors_impl_;

			auto &heap = blob.construct<VSIMHeapFloat>();
			// heap_.clear();
			// std::make_heap(std::begin(heap_), std::end(heap_));

			uint32_t iterations = 0;

			auto const nearLSH = hamming1Ranges(lsh);

			for(auto const &range : nearLSH){
				std::array<char, 5> rangeBuffer[2]; // 00FF\0

				std::string_view const rangeHash[2]{
					// add 00 in front of the hash
					hex_convert::toHex<uint16_t>(range.start,      rangeBuffer[0]),
					hex_convert::toHex<uint16_t>(range.finish + 1, rangeBuffer[1])
				};

				hm4::PairBufferKey bufferKey[2];

				auto const prefix       = P1::template makeKey(bufferKey[0], DBAdapter::SEPARATOR, keyN, /* unused */ std::string_view{}, rangeHash[0]);
				auto const prefixFinish = P1::template makeKey(bufferKey[1], DBAdapter::SEPARATOR, keyN, /* unused */ std::string_view{}, rangeHash[1]);

				logger<Logger::DEBUG>() << "VSIM.LSH" << "range" << rangeHash[0] << rangeHash[1];

				shared::stop_predicate::StopRangePredicate stop{ prefixFinish };

				auto const iterations2 = iterations;

				process_VSIM_range<T>(	db,
							stop,
							prefix,
							dim_ix,
							dtype,
							original_fvector, original_magnitude,
							results,
							heap,
							iterations
				);

				logger<Logger::DEBUG>() << "VSIM.LSH" << "range finish" << (iterations - iterations2) << "vectors";
			}

			logger<Logger::DEBUG>() << "VSIM.LSH" << "total ranges" << nearLSH.size() << "scanned total" << iterations << "vectors";

		//done: // label for goto

			return process_VSIM_finish(db, result, blob, dtype, heap, original_fvector);
		}

		static void process_bit_(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
				std::string_view keyN, uint32_t const dim_ix,
				CFVector const original_fvector, uint8_t const lsh,
				uint32_t const results ){

			using namespace vectors_impl_;

			auto const bitVectorBytesMax = Wector<bool>::bytes(MaxDimensions);

			std::array<uint8_t, bitVectorBytesMax> bitVectorBuffer;

			MyVectors::bitVectorQuantize(original_fvector, bitVectorBuffer.data());

			auto const bitVector = std::string_view{
					reinterpret_cast<const char *>(bitVectorBuffer.data()),
					MyVectors::bitVectorBytes(original_fvector.size())
			};

			auto &heap = blob.construct<VSIMHeapSize>();
			// heap_.clear();
			// std::make_heap(std::begin(heap_), std::end(heap_));

			uint32_t iterations = 0;

			auto const nearLSH = hamming1Ranges(lsh);

			for(auto const &range : nearLSH){
				std::array<char, 5> rangeBuffer[2]; // 00FF\0

				std::string_view const rangeHash[2]{
					// add 00 in front of the hash
					hex_convert::toHex<uint16_t>(range.start,      rangeBuffer[0]),
					hex_convert::toHex<uint16_t>(range.finish + 1, rangeBuffer[1])
				};

				hm4::PairBufferKey bufferKey[2];

				auto const prefix       = P1::template makeKey(bufferKey[0], DBAdapter::SEPARATOR, keyN, /* unused */ std::string_view{}, rangeHash[0]);
				auto const prefixFinish = P1::template makeKey(bufferKey[1], DBAdapter::SEPARATOR, keyN, /* unused */ std::string_view{}, rangeHash[1]);

				logger<Logger::DEBUG>() << "VSIM.LSH.BIT" << "range" << rangeHash[0] << rangeHash[1];

				shared::stop_predicate::StopRangePredicate stop{ prefixFinish };

				auto const iterations2 = iterations;

				process_VSIM_rangeBit(	db,
							stop,
							prefix,
							dim_ix,
							bitVector,
							results,
							heap,
							iterations
				);

				logger<Logger::DEBUG>() << "VSIM.LSH.BIT" << "range finish" << (iterations - iterations2) << "vectors";
			}

			logger<Logger::DEBUG>() << "VSIM.LSH.BIT" << "total ranges" << nearLSH.size() << "scanned total" << iterations << "vectors";

		//done: // label for goto

			return process_VSIM_finish(db, result, blob, DType::HAMMING, heap, original_fvector);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"vsimlsh",	"VSIMLSH"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VKSET : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// VKSET key  DIM_VE DIM_IX QUANTIZE_TYPE VEC_TYPE BLOB
		// VKSET word 300    150    F             b        BLOB
		// VKSET word 300    150    I             b        BLOB

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		I = int8

		VEC_TYPE:
		B = binary BE
		b = binary LE
		H = hex BE
		h = hex LE
		*/

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace vectors_impl_;

			if (p.size() != 7)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_6);

			auto const key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const dim_ve = from_string<uint32_t>(p[2]);

			if (dim_ve <  1 || dim_ve > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const dim_ix = from_string<uint32_t>(p[3]);

			if (dim_ix <= 1 || dim_ix > dim_ve)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[4]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const vtype = translateVType(p[5]);

			if (vtype == VType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const sizeM = translateVTypeToSizeM(vtype);

			auto const vectorSV = p[6];

			if (!MyVectors::validBlobSizeF(vectorSV.size(), dim_ve * sizeM))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			switch(qtype){
			case QType::F32	: return process_<float		>(db, result, blob, key, vectorSV, dim_ve, dim_ix, vtype);
			case QType::I16	: return process_<int16_t	>(db, result, blob, key, vectorSV, dim_ve, dim_ix, vtype);
			case QType::I8	: return process_<int8_t	>(db, result, blob, key, vectorSV, dim_ve, dim_ix, vtype);
			case QType::BIT	: return process_<bool		>(db, result, blob, key, vectorSV, dim_ve, dim_ix, vtype);

			default		: return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

	private:
		template<typename T>
		static void process_(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob, std::string_view key, std::string_view vectorSV, uint32_t const dim_ve, uint32_t const dim_ix, vectors_impl_::VType vtype){

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			using MyVKADD_Factory = VKADD_Factory<T>;

			hm4::insertV<MyVKADD_Factory>(*db, key, vectorSV, dim_ve, dim_ix, vtype, blob);

			return result.set();
		}

	private:
		template<typename T>
		struct VKADD_Factory : hm4::PairFactory::IFactoryAction<0, 0, VKADD_Factory<T> >{
			using Pair   = hm4::Pair;
			using Base   = hm4::PairFactory::IFactoryAction<0, 0, VKADD_Factory<T> >;

			constexpr VKADD_Factory(std::string_view const key, std::string_view vectorSV, uint32_t const dim_ve, uint32_t const dim_ix, vectors_impl_::VType vtype, OutputBlob &blob) :
							Base::IFactoryAction	(key, vectors_impl_::Wector<T>::bytes(dim_ix) ),
							vectorSV			(vectorSV	),
							dim_ve				(dim_ve		),
							dim_ix				(dim_ix		),
							vtype				(vtype		),
							blob				(blob		){}

			void action(Pair *pair) const{
				using namespace vectors_impl_;

				auto &wectorBuffer = *reinterpret_cast<WectorBuffer<T> *>(pair->getValC());

				[[maybe_unused]]
				const auto *w = prepareWectorBE<T>(vtype, vectorSV, dim_ve, dim_ix, wectorBuffer, blob);

				// assert(w->bytes() == pair->getVal().size() && "Size needs to be the same");
			}

		private:
			std::string_view	vectorSV;
			uint32_t		dim_ve;
			uint32_t		dim_ix;
			vectors_impl_::VType	vtype;
			OutputBlob 		&blob;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"vkset",	"VKSET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VKGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// VKGET key      DIM_IX QUANTIZE_TYPE
		// VKGET word:cat 300    F
		// VKGET word:cat 300    I

		/*
		QUANTIZE_TYPE
		F = float	e.g. do not quantize
		I = int8
		*/

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace vectors_impl_;

			auto const dim_ix = from_string<uint32_t>(p[2]);

			if (dim_ix < 1 && dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[3]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const wectorBlob = hm4::getPairVal(*db, key);

			switch(qtype){
			default:
			case QType::F32 : return process_VGET<float	>(result, blob, wectorBlob, dim_ix);
			case QType::I16 : return process_VGET<int16_t	>(result, blob, wectorBlob, dim_ix);
			case QType::I8  : return process_VGET<int8_t	>(result, blob, wectorBlob, dim_ix);
			case QType::BIT : return process_VGET<bool	>(result, blob, wectorBlob, dim_ix);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"vkget",	"VKGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VKGETRAW : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// VKGETHEX key   DIM_IX QUANTIZE_TYPE OUT_VEC_TYPE
		// VKGETHEX words 300    F             b
		// VKGETHEX words 300    I             b

		/*
		QUANTIZE_TYPE
		F = float	e.g. do not quantize
		I = int8

		VEC_TYPE:
		B = binary BE
		b = binary LE
		H = hex BE
		h = hex LE
		*/

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace vectors_impl_;

			auto const dim_ix = from_string<uint32_t>(p[2]);

			if (dim_ix < 1 && dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[3]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const vtype = translateVType(p[4]);

			if (vtype == VType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const wectorBlob = hm4::getPairVal(*db, key);

			switch(qtype){
			default:
			case QType::F32 : return process_VGETRAW<float		>(result, blob, wectorBlob, dim_ix, vtype);
			case QType::I16 : return process_VGETRAW<int16_t	>(result, blob, wectorBlob, dim_ix, vtype);
			case QType::I8  : return process_VGETRAW<int8_t		>(result, blob, wectorBlob, dim_ix, vtype);
			case QType::BIT : return process_VGETRAW<bool		>(result, blob, wectorBlob, dim_ix, vtype);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"vkgetraw",	"VKGETRAW",
			"vkgetblob",	"VKGETBLOB"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VKGETNORMALIZED : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// VKGETNORMALIZED key   DIM_IX QUANTIZE_TYPE
		// VKGETNORMALIZED words 300    F
		// VKGETNORMALIZED words 300    I

		/*
		QUANTIZE_TYPE
		F = float	e.g. do not quantize
		I = int8
		*/

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace vectors_impl_;

			auto const dim_ix = from_string<uint32_t>(p[2]);

			if (dim_ix < 1 && dim_ix > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[3]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const wectorBlob = hm4::getPairVal(*db, key);

			switch(qtype){
			default:
			case QType::F32 : return process_VGETNORMALIZED<float	>(result, blob, wectorBlob, dim_ix);
			case QType::I16 : return process_VGETNORMALIZED<int16_t	>(result, blob, wectorBlob, dim_ix);
			case QType::I8  : return process_VGETNORMALIZED<int8_t	>(result, blob, wectorBlob, dim_ix);
			case QType::BIT : return process_VGETNORMALIZED<bool	>(result, blob, wectorBlob, dim_ix);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"vkgetnormalized",	"VKGETNORMALIZED",
			"vkgetnorm",		"VKGETNORM"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VKSIMFLAT : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// VSIMFLAT prefix DIM_VE DIM_IX QUANTIZE_TYPE DISTANCE_TYPE VEC_TYPE BLOB RESULTS
		// VSIMFLAT words: 300    300    F             C             b        BLOB 100
		// VSIMFLAT words: 300    300    I             C             b        BLOB 100

		/*
		QUANTIZE_TYPE:
		F = float	e.g. do not quantize
		I = int8

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
		*/

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace vectors_impl_;

			if (p.size() != 9)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_8);

			auto const keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const dim_ve = from_string<uint32_t>(p[2]);

			if (dim_ve <  1 || dim_ve > MaxDimensions)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const dim_ix = from_string<uint32_t>(p[3]);

			if (dim_ix <= 1 || dim_ix > dim_ve)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const qtype = translateQType(p[4]);

			if (qtype == QType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const dtype = translateDType(p[5]);

			if (dtype == DType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const vtype = translateVType(p[6]);

			if (vtype == VType::UNKNOWN)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const sizeM = translateVTypeToSizeM(vtype);

			auto const vectorSV = p[7];

			if (!MyVectors::validBlobSizeF(vectorSV.size(), dim_ve * sizeM))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto &fVectorBuffer = blob.allocate<FVectorBuffer>();;

			// vectorSV size is checked already

			// uint8_t hashOut;

			/* mutable */ FVector fvector = prepareFVector(vtype, vectorSV, dim_ve, dim_ix, fVectorBuffer, blob);

			auto const magnitude = process_VSIM_prepareFVector(dtype, fvector);

			auto const results = std::clamp<uint32_t>(from_string<uint32_t>(p[8]), 1, VSIM_MAX_RESULTS);

			switch(qtype){
			case QType::F32	: return process_<float		>(db, result, blob, keyN, dim_ix, dtype, fvector, magnitude, results);
			case QType::I16	: return process_<int16_t	>(db, result, blob, keyN, dim_ix, dtype, fvector, magnitude, results);
			case QType::I8	: return process_<int8_t	>(db, result, blob, keyN, dim_ix, dtype, fvector, magnitude, results);
			case QType::BIT	: return process_bit_		 (db, result, blob, keyN, dim_ix,        fvector,            results);

			default		: return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

	private:
		template<typename T>
		static void process_(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
				std::string_view prefix, uint32_t const dim_ix,
				vectors_impl_::DType dtype,
				CFVector const original_fvector, float const original_magnitude,
				uint32_t const results ){

			using namespace vectors_impl_;

			auto &heap = blob.construct<VSIMHeapFloat>();
			// heap_.clear();
			// std::make_heap(std::begin(heap_), std::end(heap_));

			logger<Logger::DEBUG>() << "VKSIM.FLAT" << "range prefix" << prefix;

			shared::stop_predicate::StopPrefixPredicate stop{ prefix };

			uint32_t iterations = 0;

			process_VSIM_range<T>(	db,
						stop,
						prefix,
						dim_ix,
						dtype,
						original_fvector, original_magnitude,
						results,
						heap,
						iterations
			);

			logger<Logger::DEBUG>() << "VKSIM.FLAT" << "range finish" << iterations << "vectors";

			return process_VSIM_finish<1>(db, result, blob, dtype, heap, original_fvector);
		}

		void process_bit_(DBAdapter &db, Result<Protocol> &result, OutputBlob &blob,
				std::string_view prefix, uint32_t const dim_ix,
				CFVector const original_fvector,
				uint32_t const results ){

			using namespace vectors_impl_;

			auto const bitVectorBytesMax = Wector<bool>::bytes(MaxDimensions);

			auto &bitVectorBuffer = blob.allocate<std::array<uint8_t, bitVectorBytesMax> >();

			MyVectors::bitVectorQuantize(original_fvector, bitVectorBuffer.data());

			auto const bitVector = std::string_view{
					reinterpret_cast<const char *>(bitVectorBuffer.data()),
					MyVectors::bitVectorBytes(original_fvector.size())
			};

			auto &heap = blob.construct<VSIMHeapSize>();
			// heap_.clear();
			// std::make_heap(std::begin(heap_), std::end(heap_));

			logger<Logger::DEBUG>() << "VSIM.FLAT.BIT" << "range prefix" << prefix;

			shared::stop_predicate::StopPrefixPredicate stop{ prefix };

			uint32_t iterations = 0;

			process_VSIM_rangeBit(	db,
						stop,
						prefix,
						dim_ix,
						bitVector,
						results,
						heap,
						iterations
			);

			logger<Logger::DEBUG>() << "VSIM.FLAT.BIT" << "range finish" << iterations << "vectors";

			return process_VSIM_finish<1>(db, result, blob, DType::HAMMING, heap, original_fvector);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"vksimflat",	"VKSIMFLAT"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "vectors";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				VADD		,	VKSET		,
				VREM		,	/* del */
				VGET		,	VKGET		,
				VGETRAW		,	VKGETRAW	,
				VGETNORMALIZED	,	VKGETNORMALIZED	,
				VSIMFLAT	,	VKSIMFLAT	,
				VSIMLSH
			>(pack);
		}
	};



} // namespace




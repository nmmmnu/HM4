#include "base.h"
#include "minhash_raw.h"
#include "logger.h"
#include "pair_vfactory.h"
#include "shared_hint.h"
#include "stringtokenizer.h"
#include "hexconvert.h"
#include "topheap.h"

#include "shared_rset_multi.h"
#include "shared_accumulateresults.h"

namespace net::worker::commands::MH{
	namespace impl_{
		using Pair = hm4::Pair;

		constexpr size_t MHBits			= 12;

		constexpr size_t keySortSize		= 16; // uint64 as hex

		// 0031.6A796090E2DBB95E
		constexpr size_t keyBandSize		= 4 + 1 + 16; // uint32 as hex + '.' + uint64 as hex

		// keyN~keyBand~keySort~keySub
		constexpr size_t keyAdditionalSize	= /*keyN~ */   keyBandSize + 1 + keySortSize   /* ~keySub */;

		constexpr size_t tokenMinSize		= 0;

		using MHT = uint16_t;
		using MH  = minhash::MinHash<MHBits, MHT>;

		inline std::string_view MHT2sv(const MHT *mh){
			return {
				reinterpret_cast<const char *>(mh),
				MH::bytes()
			};
		}

		template<class List>
		auto store(List &list, std::string_view key, const MHT *mh){
			return hm4::insert( list,
				key,
				MHT2sv(mh)
			);
		}

		template<class List>
		const MHT *load_ptr(List &list, std::string_view key){
			if (const auto *pair = hm4::getPairPtrWithSize(list, key, MH::bytes()); pair)
				return hm4::getValAs<MHT>(pair);

			return nullptr;
		}

		template<class DBAdapter>
		const MHT *load_ptr(DBAdapter &db, std::string_view keyN, std::string_view keySub){
			hm4::PairBufferKey bufferKeyCtrl;
			auto const keyCtrl = shared::rsetmulti::makeKeyCtrl(bufferKeyCtrl,   DBAdapter::SEPARATOR, keyN, keySub);

			return load_ptr(*db, keyCtrl);
		}

		template<size_t N>
		static std::string_view formatDouble(double n, std::array<char, N> &buffer){
			constexpr static std::string_view fmt_mask = "{:+.10f}";

			auto const result = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask, n);

			if (result.out == std::end(buffer))
				return {};
			else
				return { buffer.data(), result.size };
		}

		void bandsHexToContainer(const MHT *mh_data, uint32_t bandSize, OutputBlob::Container &container, OutputBlob::BufferContainer &bcontainer){
			static_assert(MH::bytes() <= OutputBlob::ContainerSize);

			MH mh;

			auto f = [&container, &bcontainer](uint16_t id, uint64_t hash){
				bcontainer.push_back();

				char *buffer = bcontainer.back().data();

				size_t p = 0;

				// 1A421A42F0BF967F.0000 has better entropy than 0000.1A421A42F0BF967F
				// also line index works much better

				hex_convert::toHex(hash, buffer + p);	p += sizeof(uint64_t) * 2;
				buffer[p] = '.';			p += 1;
				hex_convert::toHex(id,   buffer + p);	p += sizeof(uint16_t) * 2;

				container.emplace_back(buffer, p);
			};

			mh.bands(mh_data, bandSize, f);
		}

		template<size_t N>
		auto makeKeySort(std::string_view keySub, std::array<char, N> &buffer){
			static_assert(N > keySortSize);

			return hex_convert::toHex(murmur_hash64a(keySub), buffer);
		}

		struct Decoder{
			Decoder(uint32_t bandSize) : bandSize(bandSize){}

			constexpr static auto bytes(){
				return MH::bytes();
			}

			bool operator()(std::string_view data,
						OutputBlob::Container &icontainer, OutputBlob::BufferContainer &bcontainer) const{

				if (data.size() != bytes())
					return false;

				icontainer.clear();
				bcontainer.clear();

				const MHT *mh_data = reinterpret_cast<const MHT *>(data.data());

				bandsHexToContainer(mh_data, bandSize, icontainer, bcontainer);

				return true;
			}

		private:
			uint32_t bandSize;
		};

		inline std::string_view extractNth_(size_t const nth, char const separator, std::string_view const s){
			size_t count = 0;

			for (size_t i = 0; i < s.size(); ++i)
				if (s[i] == separator)
					if (++count; count == nth)
						return s.substr(i + 1);

			return "INVALID_DATA";
		}

		inline size_t insertTokens(MH mh, MHT *mh_data, char delimiter, std::string_view tokens){
			mh.clear(mh_data);

			StringTokenizer const tok{ tokens, delimiter };

			size_t result = 0;

			for(auto const &val : tok){
				if (val.size() <= tokenMinSize)
					continue;

				result += mh.add(mh_data, val);
			}

			return result;
		}

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct MHADD : BaseCommandRW<Protocol,DBAdapter>{

		MHADD() : BaseCommandRW<Protocol,DBAdapter>("MHADD", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// MHADD key bands	keySub delimiter "words,words"
		//			keySub delimiter "words,words"

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			auto const varg  = 3;
			auto const vstep = 3;

			if (p.size() < varg + vstep || (p.size() - varg) % vstep != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_6);

			auto const keyN		= p[1];
			auto const bandSize	= from_string<uint32_t>(p[2]);

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			if (!bandSize || MH::bytes() % bandSize != 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const keySub	= *(itk + 0);
				auto const delimiter	= *(itk + 1);
			//	auto const tokens	= *(itk + 2);

				if (delimiter.size() != 1)
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

				if (keySub.empty())
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				if (!shared::rsetmulti::valid(keyN, keySub, keyAdditionalSize))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			auto &icontainer = blob.construct<OutputBlob::Container>();
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			Decoder decoder{ bandSize };

			const Pair *pair = nullptr;

			constexpr std::string_view key = ""; // will be replaces later.

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const keySub	= *(itk + 0);
				auto const delimiter	= *(itk + 1);
				auto const tokens	= *(itk + 2);

				to_string_buffer_t buffer;
				auto const keySort	= makeKeySort(keySub, buffer);

				MHSETFactory factory{ key, pair, delimiter[0], tokens, decoder, icontainer, bcontainer };

				[[maybe_unused]]
				bool const b = shared::rsetmulti::add(db, decoder,
								keyN, keySub, keySort,
									icontainer, bcontainer,
										factory);
			}

			return result.set_1();
		}

		struct MHSETFactory : hm4::PairFactory::IFactoryAction<0,1,MHSETFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<0,1,MHSETFactory>;

			MHSETFactory(std::string_view const key, const Pair *pair, char delimiter, std::string_view tokens,
								impl_::Decoder decoder,
									OutputBlob::Container &icontainer, OutputBlob::BufferContainer &bcontainer) :
							Base::IFactoryAction	(key, impl_::MH::bytes(), pair	),
							delimiter		(delimiter			),
							tokens			(tokens				),
							decoder			(decoder			),
							icontainer		(icontainer			),
							bcontainer		(bcontainer			){}

			void action(Pair *pair){
				countBits = action_(pair);
			}

			constexpr auto getCount() const{
				return countBits;
			}

			auto const &getIndexes() const{
				return icontainer;
			}

	private:
			size_t action_(Pair *pair) const{
				using namespace impl_;

				MHT *mh_data = hm4::getValAs<MHT>(pair);

				auto const result = insertTokens(MH{}, mh_data, delimiter, tokens);

				decoder(pair->getVal(), icontainer, bcontainer);

				return result;
			}

	private:
			char				delimiter;
			std::string_view		tokens;
			impl_::Decoder 			decoder;
			OutputBlob::Container		&icontainer;
			OutputBlob::BufferContainer	&bcontainer;

			size_t				countBits = 0;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mhadd",	"MHADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MHREM : BaseCommandRW<Protocol,DBAdapter>{

		MHREM() : BaseCommandRW<Protocol,DBAdapter>("MHREM", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// MHREM key bands keySub keySub..

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			auto const varg  = 3;

			if (p.size() < 4)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_4);

			auto const keyN		= p[1];
			auto const bandSize	= from_string<uint32_t>(p[2]);

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			if (!bandSize || MH::bytes() % bandSize != 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const keySub = *itk;

				if (keySub.empty())
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				if (!shared::rsetmulti::valid(keyN, keySub, keyAdditionalSize))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			auto &icontainer = blob.construct<OutputBlob::Container>();
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			Decoder decoder{ bandSize };

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const keySub	= *itk;

				to_string_buffer_t buffer;
				auto const keySort	= makeKeySort(keySub, buffer);

				[[maybe_unused]]
				bool const b = shared::rsetmulti::rem(db, decoder,
								keyN, keySub, keySort,
									icontainer, bcontainer);
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mhrem",	"MHREM"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MHGETINDEXES : BaseCommandRO<Protocol,DBAdapter>{

		MHGETINDEXES() : BaseCommandRO<Protocol,DBAdapter>("MHGETINDEXES", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// MHGETINDEXES key bands keySub
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const keyN		= p[1];
			auto const bandSize	= from_string<uint32_t>(p[2]);
			auto const keySub	= p[3];

			using namespace impl_;

			if (!shared::rsetmulti::valid(keyN, keySub, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			if (!bandSize || MH::bytes() % bandSize != 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto &icontainer = blob.construct<OutputBlob::Container>();
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			Decoder decoder{ bandSize };

			[[maybe_unused]]
			bool const b = shared::rsetmulti::getIndexes(db, decoder,
							keyN, keySub,
								icontainer, bcontainer);

			auto const &container = icontainer;

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mhgetindexes",	"MHGETINDEXES"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MHSIM : BaseCommandRO<Protocol,DBAdapter>{

		MHSIM() : BaseCommandRO<Protocol,DBAdapter>("MHSIM", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// MHSIM key bands keySub
		// MHSIM key bands delimiter "words,words"
		//
		// using:
		//	icontainer, reused as result  container
		//	bcontainer, reused as result bcontainer
		//	keySub_container
		//	heap
		//	MHT[MH::size()];

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() != 4 && p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			bool const inputTypeIsKey = p.size() == 4;

			if (inputTypeIsKey)
				return process2__<1>(p, db, result, blob);
			else
				return process2__<0>(p, db, result, blob);
		}

		template<bool inputTypeIsKey>
		static void process2__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			auto const keyN		= p[1];
			auto const bandSize	= from_string<uint32_t>(p[2]);

			auto const keySub	=  inputTypeIsKey ? p[3] : "";
			auto const delimiter	= !inputTypeIsKey ? p[3] : "";
			auto const tokens	= !inputTypeIsKey ? p[4] : "";

			using namespace impl_;

			if (!bandSize || MH::bytes() % bandSize != 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			if constexpr(inputTypeIsKey){
				if (!shared::rsetmulti::valid(keyN, keySub, keyAdditionalSize))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}else{

				if (delimiter.size() != 1)
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}

			auto &icontainer = blob.construct<OutputBlob::Container>();
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			Decoder decoder{ bandSize };

			MH mh;

			const MHT *mhA = [&]() -> const MHT *{
				if constexpr(inputTypeIsKey){
					return load_ptr(db, keyN, keySub);
				}else{
					MHT *mh_data = & blob.allocate<MHT>(MH::bytes());

					if (insertTokens(mh, mh_data, delimiter[0], tokens))
						return mh_data;
					else
						return nullptr;
				}
			}();

			if (!mhA)
				return result.set_container0();

			// getIndexes - decode indexes manually, to avoid disk operation
			if (!decoder(MHT2sv(mhA), icontainer, bcontainer))
				return result.set_container0();

			auto &keySub_container  = blob.construct<OutputBlob::Container>();

			for(auto const &index : icontainer){
				hm4::PairBufferKey bufferKey;
				auto const prefix = shared::rsetmulti::makeKeyDataSearch(bufferKey, DBAdapter::SEPARATOR, keyN, index);

				scanIndex__(db, prefix, count__, keySub_container);
			}

			// icontainer and bcontainer no longer need.

			std::sort(std::begin(keySub_container), std::end(keySub_container));
			auto uniq_end = std::unique(std::begin(keySub_container), std::end(keySub_container));

			using HeapNode = std::pair<double, std::string_view>;
			auto &heap = blob.construct<top_heap::TopKLargest<HeapNode, results__> >();

			for(auto it = std::begin(keySub_container); it != uniq_end; ++it){
				auto const &text = *it;

				if constexpr(inputTypeIsKey){
					if (text == keySub)
						continue;
				}

				const auto *mhB = load_ptr(db, keyN, text);

				if (!mhB)
					continue;

				auto const jaccard = mh.jaccard(mhA, mhB);

				if (jaccard < minScore__)
					continue;

				heap.push(HeapNode{ jaccard, text });
			}

			// keySub_container, icontainer and bcontainer no longer need.

			auto &container  = icontainer;

			container.clear();
			bcontainer.clear();

			auto &data = heap.sort();

		//	std::sort(std::begin(data), std::end(data), std::greater{});

			for(auto &[jaccard, text] : data){
				container.push_back(text);

				bcontainer.push_back();

				container.push_back(
					formatDouble(
						jaccard,
						bcontainer.back()
					)
				);
			}

			return result.set_container(container);
		}

		static void scanIndex__(DBAdapter &db, std::string_view prefix, uint32_t count, OutputBlob::Container &container){
			using namespace shared::accumulate_results;

			auto const &key = prefix;

			logger<Logger::DEBUG>() << "MHSIM" << "prefix" << prefix << "key" << key;

			StopPrefixPredicate stop{ prefix };

			auto proj = [](std::string_view x){
				[[maybe_unused]]
				auto const separator = DBAdapter::SEPARATOR[0];

				// keyN~word~keySort~keySub
				return impl_::extractNth_(3, separator, x);
			};

			auto const Out = AccumulateOutput::KEYS;

			sharedAccumulateResults<Out>(
				count		,
				stop		,
				db->find(key)	,
				std::end(*db)	,
				container	,
				proj
			);
		}

	private:
		constexpr static size_t count__		= 32;
		constexpr static size_t results__	= 32;
		constexpr static double minScore__	= 0.05;

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mhsim",	"MHSIM"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MHJACCARD : BaseCommandRO<Protocol,DBAdapter>{

		MHJACCARD() : BaseCommandRO<Protocol,DBAdapter>("MHJACCARD", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		// MHJACCARD key bands keySub keySub2
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const keyN		= p[1];
			auto const bandSize	= from_string<uint32_t>(p[2]);
			auto const keySubA	= p[3];
			auto const keySubB	= p[4];

			using namespace impl_;

			if (keyN.empty() || keySubA.empty() || keySubB.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!shared::rsetmulti::valid(keyN, keySubA, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			if (!shared::rsetmulti::valid(keyN, keySubB, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			if (!bandSize || MH::bytes() % bandSize != 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			const auto *mhA = load_ptr(db, keyN, keySubA);

			if (!mhA)
				return result.set_0();

			const auto *mhB = load_ptr(db, keyN, keySubB);

			if (!mhB)
				return result.set_0();

			MH mh;

			to_string_buffer_t buffer;

			return result.set(
				formatDouble(
					mh.jaccard(mhA, mhB),
					buffer
				)
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mhjaccard",	"MHJACCARD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MHMJACCARD : BaseCommandRO<Protocol,DBAdapter>{

		MHMJACCARD() : BaseCommandRO<Protocol,DBAdapter>("MHMJACCARD", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// MHMJACCARD key bands keySub keySub2...
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){

			if (p.size() < 5)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_4);

			auto const varg = 4;

			auto const keyN		= p[1];
			auto const bandSize	= from_string<uint32_t>(p[2]);
			auto const keySubA	= p[3];

			if (keyN.empty() || keySubA.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			if (!shared::rsetmulti::valid(keyN, keySubA, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			if (!bandSize || MH::bytes() % bandSize != 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &keySub = *itk;

				if (keySub.empty())
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				if (!shared::rsetmulti::valid(keyN, keySub, keyAdditionalSize))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			auto &container = blob.construct<OutputBlob::Container>();

			const auto *mhA = load_ptr(db, keyN, keySubA);

			if (!mhA){
				// main MinHash does not exists

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
					container.push_back("0");

				return result.set_container(container);
			}

			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			MH mh;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto *mhB = load_ptr(db, keyN, *itk);

				if (!mhB){
					container.push_back("0");

					continue;
				}

				bcontainer.push_back();

				container.push_back(
					formatDouble(
						mh.jaccard(mhA, mhB),
						bcontainer.back()
					)
				);
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mhmjaccard",	"MHMJACCARD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MHOVERLAP : BaseCommandRO<Protocol,DBAdapter>{

		MHOVERLAP() : BaseCommandRO<Protocol,DBAdapter>("MHOVERLAP", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		// MHOVERLAP key bands keySub keySub2
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const keyN		= p[1];
			auto const bandSize	= from_string<uint32_t>(p[2]);
			auto const keySubA	= p[3];
			auto const keySubB	= p[4];

			using namespace impl_;

			if (keyN.empty() || keySubA.empty() || keySubB.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!shared::rsetmulti::valid(keyN, keySubA, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			if (!shared::rsetmulti::valid(keyN, keySubB, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			if (!bandSize || MH::bytes() % bandSize != 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			const auto *mhA = load_ptr(db, keyN, keySubA);

			if (!mhA)
				return result.set_0();

			const auto *mhB = load_ptr(db, keyN, keySubB);

			if (!mhB)
				return result.set_0();

			MH mh;

			to_string_buffer_t buffer;

			return result.set(
				formatDouble(
					mh.overlap(mhA, mhB),
					buffer
				)
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mhoverlap",	"MHOVERLAP"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MHMOVERLAP : BaseCommandRO<Protocol,DBAdapter>{

		MHMOVERLAP() : BaseCommandRO<Protocol,DBAdapter>("MHMOVERLAP", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// MHMOVERLAP key bands keySub keySub2...
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){

			if (p.size() < 5)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_4);

			auto const varg = 4;

			auto const keyN		= p[1];
			auto const bandSize	= from_string<uint32_t>(p[2]);
			auto const keySubA	= p[3];

			if (keyN.empty() || keySubA.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			if (!shared::rsetmulti::valid(keyN, keySubA, keyAdditionalSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			if (!bandSize || MH::bytes() % bandSize != 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &keySub = *itk;

				if (keySub.empty())
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				if (!shared::rsetmulti::valid(keyN, keySub, keyAdditionalSize))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			auto &container = blob.construct<OutputBlob::Container>();

			const auto *mhA = load_ptr(db, keyN, keySubA);

			if (!mhA){
				// main MinHash does not exists

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
					container.push_back("0");

				return result.set_container(container);
			}

			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			MH mh;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto *mhB = load_ptr(db, keyN, *itk);

				if (!mhB){
					container.push_back("0");

					continue;
				}

				bcontainer.push_back();

				container.push_back(
					formatDouble(
						mh.overlap(mhA, mhB),
						bcontainer.back()
					)
				);
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mhmoverlap",	"MHMOVERLAP"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MHBITS : BaseCommandRO<Protocol,DBAdapter>{

		MHBITS() : BaseCommandRO<Protocol,DBAdapter>("MHBITS", std::begin(cmd__), std::end(cmd__)){}

		constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			using namespace impl_;

			return result.set(uint64_t{ MHBits });
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mhbits",	"MHBITS"
		};

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mh";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				MHADD		,
				MHREM		,
				MHGETINDEXES	,
				MHSIM		,
				MHJACCARD	,
				MHMJACCARD	,
				MHOVERLAP	,
				MHMOVERLAP	,
				MHBITS
			>(pack);
		}
	};

} // namespace



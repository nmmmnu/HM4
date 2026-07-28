#include "base.h"
#include "minhash_raw.h"
#include "logger.h"
#include "pair_vfactory.h"
#include "shared_hint.h"
#include "stringtokenizer.h"

namespace net::worker::commands::MH{
	namespace impl_{
		using Pair = hm4::Pair;

		constexpr auto MHBits = 12u;

		using MHT = uint16_t;
		using MH  = minhash::MinHash<MHBits, MHT>;



		template<class List>
		auto store(List &list, std::string_view key, const MHT *mh){
			return hm4::insert( list,
				key,
				std::string_view{
					reinterpret_cast<const char *>(mh),
					MH::bytes()
				}
			);
		}

		template<class List>
		const MHT *load_ptr(List &list, std::string_view key){
			if (const auto *pair = hm4::getPairPtrWithSize(list, key, MH::bytes()); pair)
				return hm4::getValAs<MHT>(pair);

			return nullptr;
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

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct MHADD : BaseCommandRW<Protocol,DBAdapter>{

		MHADD() : BaseCommandRW<Protocol,DBAdapter>("MHADD", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		// MHADD key val val...
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			using namespace impl_;

			const auto *pair = hm4::getPairPtrWithSize(*db, key, MH::bytes());

			MHADDFactory factory{ key, pair, std::begin(p) + varg, std::end(p) };

			insertHintVFactory(*db, pair, factory);

			return result.set(factory.getBits());
		}

		struct MHADDFactory : hm4::PairFactory::IFactoryAction<1,1,MHADDFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,MHADDFactory>;

			using It   = ParamContainer::iterator;

			MHADDFactory(std::string_view const key, const Pair *pair, It begin, It end) :
							Base::IFactoryAction	(key, impl_::MH::bytes(), pair	),
							begin			(begin				),
							end			(end				){}

			void action(Pair *pair){
				bits = action_(pair);
			}

			constexpr auto getBits() const{
				return bits;
			}



			bool action_(Pair *pair) const{
				using namespace impl_;

				MHT *mh_data = hm4::getValAs<MHT>(pair);

				auto mh = MH{};

				bool result = false;

				for(auto itk = begin; itk != end; ++itk)
					result |= mh.add(mh_data, *itk);

				return result;
			}

			It	begin;
			It	end;

			bool	bits = false;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mhadd",	"MHADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MHSET : BaseCommandRW<Protocol,DBAdapter>{

		MHSET() : BaseCommandRW<Protocol,DBAdapter>("MHSET", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		// MHSET key delimiter "words,words"
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() < 4)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const delimiter = p[2];

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const tokens = p[3];

			// not checking size

			using namespace impl_;

			const Pair *pair = nullptr;

			MHSETFactory factory{ key, pair, delimiter[0], tokens };

			insertHintVFactory(*db, pair, factory);

			return result.set(factory.getCount());
		}

		struct MHSETFactory : hm4::PairFactory::IFactoryAction<0,1,MHSETFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<0,1,MHSETFactory>;

			MHSETFactory(std::string_view const key, const Pair *pair, char delimiter, std::string_view tokens) :
							Base::IFactoryAction	(key, impl_::MH::bytes(), pair	),
							delimiter		(delimiter			),
							tokens			(tokens				){}

			void action(Pair *pair){
				countBits = action_(pair);
			}

			constexpr auto getCount() const{
				return countBits;
			}



			size_t action_(Pair *pair) const{
				using namespace impl_;

				MHT *mh_data = hm4::getValAs<MHT>(pair);

				auto mh = MH{};

				mh.clear(mh_data);

				StringTokenizer const tok{ tokens, delimiter };

				bool result = 0;

				for(auto const &val : tok){
					if (!val.size())
						continue;

					result += mh.add(mh_data, val) ? 1 : 0;
				}

				return result;
			}

			char			delimiter;
			std::string_view	tokens;

			size_t			countBits = 0;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mhset",	"MHSET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MHRESERVE : BaseCommandRW<Protocol,DBAdapter>{

		MHRESERVE() : BaseCommandRW<Protocol,DBAdapter>("MHRESERVE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() < 2)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_1);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			hm4::insertV<hm4::PairFactory::Reserve>(*db, key, MH::bytes());

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mhreserve",	"MHRESERVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MHJACCARD : BaseCommandRO<Protocol,DBAdapter>{

		MHJACCARD() : BaseCommandRO<Protocol,DBAdapter>("MHJACCARD", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		// MHJACCARD key otherkey
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto &otherKey = p[2];

			if (!hm4::Pair::isKeyValid(otherKey))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			const auto *mhA = load_ptr(*db, key);

			if (!mhA)
				return result.set_0();

			const auto *mhB = load_ptr(*db, otherKey);

			if (!mhB)
				return result.set_0();

			auto mh = MH{};

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
			return process_(p, db, result, blob);
		}

	private:
		// MHJACCARD key otherkey...
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){

			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			auto const varg = 2;

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			auto &container = blob.construct<OutputBlob::Container>();

			const auto *mhA = load_ptr(*db, key);

			if (!mhA){
				// main MinHash does not exists

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
					container.push_back("0");

				return result.set_container(container);
			}

			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			auto mh = MH{};

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto *mhB = load_ptr(*db, *itk);

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
			return process_(p, db, result);
		}

	private:
		// MHOVERLAP key otherkey
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto &otherKey = p[2];

			if (!hm4::Pair::isKeyValid(otherKey))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			const auto *mhA = load_ptr(*db, key);

			if (!mhA)
				return result.set_0();

			const auto *mhB = load_ptr(*db, otherKey);

			if (!mhB)
				return result.set_0();

			auto mh = MH{};

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
			return process_(p, db, result, blob);
		}

	private:
		// MHMOVERLAP key otherkey...
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){

			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			auto const varg = 2;

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			auto &container = blob.construct<OutputBlob::Container>();

			const auto *mhA = load_ptr(*db, key);

			if (!mhA){
				// main MinHash does not exists

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
					container.push_back("0");

				return result.set_container(container);
			}

			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			auto mh = MH{};

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto *mhB = load_ptr(*db, *itk);

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
	struct MHMERGE : BaseCommandRW<Protocol,DBAdapter>{

		MHMERGE() : BaseCommandRW<Protocol,DBAdapter>("MHMERGE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		// MHMERGE key otherkey otherkey...
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			MHVector container;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &src_key = *itk;

				// prevent merge with itself.
				if (src_key == key)
					continue;

				if (const auto *b = load_ptr(*db, src_key); b)
					container.push_back(b);
			}

			if (container.empty())
				return result.set();

			const auto *pair = hm4::getPairPtrWithSize(*db, key, MH::bytes());

			MHMergeFactory factory{ key, pair, std::begin(container), std::end(container) };

			insertHintVFactory(*db, pair, factory);

			return result.set();
		}

		using MHVector = StaticVector<const impl_::MHT *, OutputBlob::ParamContainerSize>;

		struct MHMergeFactory : hm4::PairFactory::IFactoryAction<1,1,MHMergeFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,MHMergeFactory>;

			using It = MHVector::iterator;

			MHMergeFactory(std::string_view const key, const Pair *pair, It begin, It end) :
							Base::IFactoryAction	(key, impl_::MH::bytes(), pair	),
							begin			(begin				),
							end			(end				){}

			void action(Pair *pair) const{
				using namespace impl_;

				MHT *mh_data = hm4::getValAs<MHT>(pair);

				auto mh = MH{};

				// This is fine, because flush list give guarantees now.

				for(auto it = begin; it != end; ++it)
					mh.merge(mh_data, *it);
			}

			It	begin;
			It	end;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mhmerge",	"MHMERGE"
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
				MHSET		,
				MHRESERVE	,
				MHJACCARD	,
				MHMJACCARD	,
				MHOVERLAP	,
				MHMOVERLAP	,
				MHMERGE		,
				MHBITS
			>(pack);
		}
	};


} // namespace



#include "base.h"

//#include "isam.h"
#include "isam_hashsearcher.h"

#include "my_type_traits.h"

namespace net::worker::commands::ISAM_cmd{

	constexpr bool UseHashSearcher = true;

	namespace ISAM_impl_{

		constexpr bool selectFastSearcher(size_t size, size_t searches){
			constexpr size_t M    = std::numeric_limits<size_t>::max();
			constexpr size_t SIZE = 4;
			constexpr size_t LINEAR[SIZE + 1] { M, M, 10, 4, 3 };

			auto const id = searches < SIZE ? searches : SIZE;

			return size >= LINEAR[id];
		}

		template<typename size_type>
		void logme_(std::string_view component, std::string_view searcher, ISAM const &isam, size_type expected){
			logger<Logger::DEBUG>() << component << searcher << isam.size() << expected;
		}

		inline auto getFastSearcher(ISAM const &isam, std::string_view component, size_t expected){
			if constexpr(UseHashSearcher){
				logme_(component, "hash", isam, expected);
				return isam.getHashSearcherByName();
			}else{
				logme_(component, "log", isam, expected);
				return isam.getIndexSearcherByName();
			}
		}

		enum class IGETALLOutput{
			KEYS,
			VALS,
			BOTH
		};

		template<IGETALLOutput Out, class Protocol, class DBAdapter>
		void IGETALL_process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &schema = p[1];
			auto const &key    = p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			ISAM const isam{ schema };

			auto &container = blob.construct<OutputBlob::Container>();

			constexpr size_t multiplier = [](){
				switch(Out){
				default:
				case IGETALLOutput::BOTH : return 2;
				case IGETALLOutput::KEYS :
				case IGETALLOutput::VALS : return 1;
				}
			}();

			if (container.capacity() < isam.size() * multiplier)
				return result.set_error(ResultErrorMessages::CONTAINER_CAPACITY);

			if constexpr(Out == IGETALLOutput::KEYS){
				// no need fetching the info

				for(size_t i = 0; i < isam.size(); ++i)
					container.push_back(isam.getName(i));

			}else{
				const auto *pair = hm4::getPairPtrWithSize(*db, key, isam.bytes());

				if (!pair)
					return result.set_container0();

				const char *storage  = pair->getValC();

				for(size_t i = 0; i < isam.size(); ++i){
					if constexpr(is_any_of(Out, IGETALLOutput::BOTH, IGETALLOutput::KEYS))
						container.push_back(isam.getName(i));

					if constexpr(is_any_of(Out, IGETALLOutput::BOTH, IGETALLOutput::VALS))
						container.push_back(isam.load(storage, i));
				}
			}

			return result.set_container(container);
		}

	} // namespace ISAM_impl_


	template<class Protocol, class DBAdapter>
	struct IGETALL : BaseCommandRO<Protocol,DBAdapter>{

		IGETALL() : BaseCommandRO<Protocol,DBAdapter>("IGETALL", std::begin(cmd__), std::end(cmd__)){}

		// igetall schema key
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace ISAM_impl_;

			return IGETALL_process<IGETALLOutput::BOTH>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"igetall",		"IGETALL"
		};

	};



	template<class Protocol, class DBAdapter>
	struct IGETKEYS: BaseCommandRO<Protocol,DBAdapter>{

		IGETKEYS() : BaseCommandRO<Protocol,DBAdapter>("IGETKEYS", std::begin(cmd__), std::end(cmd__)){}

		// igetall schema key
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace ISAM_impl_;

			return IGETALL_process<IGETALLOutput::KEYS>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"igetkeys",		"IGETKEYS"
		};

	};



	template<class Protocol, class DBAdapter>
	struct IGETVALS: BaseCommandRO<Protocol,DBAdapter>{

		IGETVALS() : BaseCommandRO<Protocol,DBAdapter>("IGETVALS", std::begin(cmd__), std::end(cmd__)){}

		// igetall schema key
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace ISAM_impl_;

			return IGETALL_process<IGETALLOutput::VALS>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"igetvals",		"IGETVALS"
		};

	};



	template<class Protocol, class DBAdapter>
	struct IGET : BaseCommandRO<Protocol,DBAdapter>{

		IGET() : BaseCommandRO<Protocol,DBAdapter>("IGET", std::begin(cmd__), std::end(cmd__)){}

		// iget schema key subkey
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &schema = p[1];
			auto const &key    = p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const &column = p[3];

			if (!hm4::Pair::isKeyValid(column))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const &storage  = hm4::getPairVal(*db, key);

			// similar to IMGET::collect_1__

			auto const &[isam, searcher] = ISAM::createAndSearchByName(schema, column);

			if (isam.bytes() != storage.size())
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			return result.set(
				isam.load(storage.data(), searcher)
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"iget",		"IGET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct IMGET : BaseCommandRO<Protocol,DBAdapter>{

		IMGET() : BaseCommandRO<Protocol,DBAdapter>("IMGET", std::begin(cmd__), std::end(cmd__)){}

		// imget schema key subkey subkey...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 4)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			auto const &schema = p[1];
			auto const &key    = p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 3;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const &storage  = hm4::getPairVal(*db, key);

			auto &container = blob.construct<OutputBlob::Container>();

			if (p.size() - varg == 1)
				return collect_1__(result, container, schema, storage, std::begin(p) + varg, std::end(p));
			else
				return collect_N__(result, container, schema, storage, std::begin(p) + varg, std::end(p));
		}


		template<typename Container>
		static void collect_1__(Result<Protocol> &result, Container &container,
							std::string_view schema, std::string_view storage,
							ParamContainer::const_iterator begin, ParamContainer::const_iterator end){
			using namespace ISAM_impl_;

			auto const &[isam, searcher] = ISAM::createAndSearchByName(schema, *begin);

			if (isam.bytes() != storage.size())
				return result.set_error(ResultErrorMessages::CONTAINER_CAPACITY);

			logme_("IMGET", "single", isam, 1);

			return collect__(isam, searcher, result, container, storage, begin, end);
		}

		template<typename Container>
		static void collect_N__(Result<Protocol> &result, Container &container,
							std::string_view schema, std::string_view storage,
							ParamContainer::const_iterator begin, ParamContainer::const_iterator end){
			using namespace ISAM_impl_;

			ISAM const isam{ schema };

			if (isam.bytes() != storage.size())
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const expected = std::distance(begin, end);

			if (!selectFastSearcher(isam.size(), static_cast<size_t>(expected))){
				logme_("IMGET", "linear", isam, expected);
				auto searcher = isam.getLinearSearcherByName();

				return collect__(isam, searcher, result, container, storage, begin, end);
			}else{
				auto searcher = getFastSearcher(isam, "IMGET", static_cast<size_t>(expected));

				return collect__(isam, searcher, result, container, storage, begin, end);
			}
		}

		template<typename Container, typename Searcher>
		static void collect__(ISAM const &isam, Searcher const &searcher,
						Result<Protocol> &result, Container &container,
							std::string_view storage,
							ParamContainer::const_iterator begin, ParamContainer::const_iterator end){
			for(auto itk = begin; itk != end; ++itk)
				container.push_back( isam.load(storage.data(), searcher, *itk) );

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"imget",		"IMGET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct ISETALL : BaseCommandRW<Protocol,DBAdapter>{

		ISETALL() : BaseCommandRW<Protocol,DBAdapter>("ISETALL", std::begin(cmd__), std::end(cmd__)){}

		// isetall schema key value value ...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() < 4)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			auto const &schema = p[1];
			auto const &key    = p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 3;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			ISAM const isam{ schema };

			if (isam.size() != p.size() - varg)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			hm4::insertV<ISETALL_Factory>(*db, key, isam, std::begin(p) + varg, std::end(p));

			return result.set_1();
		}


		struct ISETALL_Factory : hm4::PairFactory::IFactoryAction<1,1, ISETALL_Factory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1, ISETALL_Factory>;
			using It   = ParamContainer::const_iterator;

			constexpr ISETALL_Factory(std::string_view const key, ISAM const &isam, It begin, [[maybe_unused]] It end) :
							Base::IFactoryAction	(key, isam.bytes()),
							isam			(isam	),
							it			(begin	){

				// double check, just in case
				assert(isam.size() == static_cast<size_t>(std::distance(begin, end)));
			}

			void action(Pair *pair) const{
				char *storage = pair->getValC();

				for (size_t i = 0; i < isam.size(); ++i)
					isam.store(storage, i, *(it + i));
			}


			ISAM const	&isam;
			It		it;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"isetall",		"ISETALL"
		};

	};



	template<class Protocol, class DBAdapter>
	struct ISET : BaseCommandRW<Protocol,DBAdapter>{

		ISET() : BaseCommandRW<Protocol,DBAdapter>("ISET", std::begin(cmd__), std::end(cmd__)){}

		// iset schema key subkey value subkey value ...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			auto const varg = 3;

			if (p.size() < 5 || (p.size() - varg) % 2 != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_4);

			auto const &schema = p[1];
			auto const &key    = p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (p.size() - varg == 2)
				return process_1__(db, result, schema, key, std::begin(p) + varg, std::end(p));
			else
				return process_N__(db, result, schema, key, std::begin(p) + varg, std::end(p));
		}


		static void process_1__(DBAdapter &db, Result<Protocol> &result,
				std::string_view const schema, std::string_view const key,
								ParamContainer::const_iterator begin, ParamContainer::const_iterator end){
			using namespace ISAM_impl_;

			auto const &[isam, searcher] = ISAM::createAndSearchByName(schema, *begin);

			const auto *pair = hm4::getPairPtrWithSize(*db, key, isam.bytes());

			logme_("ISET", "single", isam, 1);

			return process__(db, result, key, pair, isam, searcher, begin, end);
		}

		static void process_N__(DBAdapter &db, Result<Protocol> &result,
				std::string_view const schema, std::string_view const key,
								ParamContainer::const_iterator begin, ParamContainer::const_iterator end){
			using namespace ISAM_impl_;

			ISAM const isam{ schema };

			const auto *pair = hm4::getPairPtrWithSize(*db, key, isam.bytes());

			auto const expected = std::distance(begin, end);

			if (!selectFastSearcher(isam.size(), static_cast<size_t>(expected))){
				logme_("ISET", "linear", isam, expected);
				auto searcher = isam.getLinearSearcherByName();

				return process__(db, result, key, pair, isam, searcher, begin, end);
			}else{
				auto searcher = getFastSearcher(isam, "ISET", static_cast<size_t>(expected));

				return process__(db, result, key, pair, isam, searcher, begin, end);
			}
		}

		template<typename Searcher>
		static void process__(DBAdapter &db, Result<Protocol> &result,
				std::string_view const key, const hm4::Pair *pair, ISAM const &isam, Searcher const &searcher,
								ParamContainer::const_iterator begin, ParamContainer::const_iterator end){

			using MyISET_Factory = ISET_Factory<Searcher>;

			MyISET_Factory factory{ key, pair, isam, searcher, begin, end };

			insertHintVFactory(*db, pair, factory);

			return result.set(
				factory.getResult()
			);
		}

		template<typename Searcher>
		struct ISET_Factory : hm4::PairFactory::IFactoryAction<1,1, ISET_Factory<Searcher> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1, ISET_Factory<Searcher> >;
			using It   = ParamContainer::const_iterator;

			constexpr ISET_Factory(std::string_view const key, const Pair *pair, ISAM const &isam, Searcher const &searcher, It begin, It end) :
							Base::IFactoryAction	(key, isam.bytes(), pair, ISAM::PADDING),
							isam			(isam		),
							searcher		(searcher	),
							begin			(begin		),
							end			(end		){}

			void action(Pair *pair){
				char *storage = pair->getValC();

				for (auto itk = begin; itk != end; itk += 2){
					auto const &key   = *(itk + 0);
					auto const &value = *(itk + 1);

					status |= isam.store(storage, value, searcher, key);
				}
			}

			bool getResult() const{
				return status;
			}


			ISAM const	&isam;
			Searcher const	&searcher;
			It		begin;
			It		end;
			bool		status		= false;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"iset",		"ISET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct IDEL : BaseCommandRW<Protocol,DBAdapter>{

		IDEL() : BaseCommandRW<Protocol,DBAdapter>("IDEL", std::begin(cmd__), std::end(cmd__)){}

		// idel schema key subkey subkey...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			auto const varg = 3;

			if (p.size() < 4)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_4);

			auto const &schema = p[1];
			auto const &key    = p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (p.size() - varg == 1)
				return process_1__(db, result, schema, key, std::begin(p) + varg, std::end(p));
			else
				return process_N__(db, result, schema, key, std::begin(p) + varg, std::end(p));
		}


		static void process_1__(DBAdapter &db, Result<Protocol> &result,
				std::string_view const schema, std::string_view const key,
							ParamContainer::const_iterator begin, ParamContainer::const_iterator end){
			using namespace ISAM_impl_;

			auto const &[isam, searcher] = ISAM::createAndSearchByName(schema, *begin);

			const auto *pair = hm4::getPairPtrWithSize(*db, key, isam.bytes());

			logme_("IDEL", "single", isam, 1);

			return process__(db, result, key, pair, isam, searcher, begin, end);
		}

		static void process_N__(DBAdapter &db, Result<Protocol> &result,
				std::string_view const schema, std::string_view const key,
							ParamContainer::const_iterator begin, ParamContainer::const_iterator end){
			using namespace ISAM_impl_;

			ISAM const isam{ schema };

			const auto *pair = hm4::getPairPtrWithSize(*db, key, isam.bytes());

			auto const expected = std::distance(begin, end);

			if (!selectFastSearcher(isam.size(), static_cast<size_t>(expected))){
				logme_("IDEL", "linear", isam, expected);
				auto searcher = isam.getLinearSearcherByName();

				return process__(db, result, key, pair, isam, searcher, begin, end);
			}else{
				auto searcher = getFastSearcher(isam, "IDEL", static_cast<size_t>(expected));

				return process__(db, result, key, pair, isam, searcher, begin, end);
			}
		}

		template<typename Searcher>
		static void process__(DBAdapter &db, Result<Protocol> &result,
				std::string_view const key, const hm4::Pair *pair, ISAM const &isam, Searcher const &searcher,
						ParamContainer::const_iterator begin, ParamContainer::const_iterator end){

			using MyIDEL_Factory = IDEL_Factory<Searcher>;

			MyIDEL_Factory factory{ key, pair, isam, searcher, begin, end };

			insertHintVFactory(*db, pair, factory);

			return result.set(
				factory.getResult()
			);
		}

		template<typename Searcher>
		struct IDEL_Factory : hm4::PairFactory::IFactoryAction<1,1, IDEL_Factory<Searcher> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1, IDEL_Factory<Searcher> >;
			using It   = ParamContainer::const_iterator;

			constexpr IDEL_Factory(std::string_view const key, const Pair *pair, ISAM const &isam, Searcher const &searcher, It begin, It end) :
							Base::IFactoryAction	(key, isam.bytes(), pair, ISAM::PADDING),
							isam			(isam		),
							searcher		(searcher	),
							begin			(begin		),
							end			(end		){}

			void action(Pair *pair){
				char *storage = pair->getValC();



				for (auto itk = begin; itk != end; ++itk){
					auto const &key   = *itk;
					auto const &value = std::string_view{};

					status |= isam.store(storage, value, searcher, key);
				}
			}

			bool getResult() const{
				return status;
			}


			ISAM const	&isam;
			Searcher const	&searcher;
			It		begin;
			It		end;
			bool		status		= false;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"idel",		"IDEL"
		};

	};



	template<class Protocol, class DBAdapter>
	struct IRESERVE : BaseCommandRW<Protocol,DBAdapter>{

		IRESERVE() : BaseCommandRW<Protocol,DBAdapter>("IRESERVE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			auto const &schema = p[1];
			auto const &key    = p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			ISAM const isam{ schema };

			hm4::insertV<hm4::PairFactory::Reserve>(*db, key, isam.bytes(), ISAM::PADDING);

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ireserve",	"IRESERVE"
		};

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "isam";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				IGETALL		,
				IGETKEYS	,
				IGETVALS	,
				IGET		,
				IMGET		,
				ISETALL		,
				ISET		,
				IDEL		,
				IRESERVE
			>(pack);
		}
	};

} // namespace



#include "base.h"
#include "isam.h"
#include "my_type_traits.h"

namespace net::worker::commands::ISAM_cmd{

	constexpr size_t LINEAR_SEARCH_MAX = 4;


	namespace ISAM_impl_{

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
					return result.set_container(container);

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
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// igetall schema key
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace ISAM_impl_;

			return IGETALL_process<IGETALLOutput::BOTH>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"igetall",		"IGETALL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IGETKEYS: BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// igetall schema key
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace ISAM_impl_;

			return IGETALL_process<IGETALLOutput::KEYS>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"igetkeys",		"IGETKEYS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IGETVALS: BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// igetall schema key
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace ISAM_impl_;

			return IGETALL_process<IGETALLOutput::VALS>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"igetvals",		"IGETVALS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

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
		constexpr inline static std::string_view cmd[]	= {
			"iget",		"IGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IMGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// imget schema key subkey subkey...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 4)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			auto const &schema = p[1];
			auto const &key    = p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto &container = blob.construct<OutputBlob::Container>();

			auto const varg = 3;

			if (container.capacity() < p.size() - varg)
				return result.set_error(ResultErrorMessages::CONTAINER_CAPACITY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const &storage  = hm4::getPairVal(*db, key);

			if (p.size() - varg == 1)
				return collect_1__(result, container, schema, storage, std::begin(p) + varg, std::end(p));
			else
				return collect_N__(result, container, schema, storage, std::begin(p) + varg, std::end(p));
		}

	private:
		template<typename Container, typename It>
		static void collect_1__(Result<Protocol> &result, Container &container, std::string_view schema, std::string_view storage, It begin, It end){
			auto const &[isam, searcher] = ISAM::createAndSearchByName(schema, *begin);

			if (isam.bytes() != storage.size())
				return result.set_error(ResultErrorMessages::CONTAINER_CAPACITY);

			return collect__(isam, searcher, result, container, storage, begin, end);
		}

		template<typename Container, typename It>
		static void collect_N__(Result<Protocol> &result, Container &container, std::string_view schema, std::string_view storage, It begin, It end){
			ISAM const isam{ schema };

			if (isam.bytes() != storage.size())
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			if (isam.size() <= LINEAR_SEARCH_MAX){
				auto searcher = isam.getLinearSearcherByName();
				return collect__(isam, searcher, result, container, storage, begin, end);
			}else{
				auto searcher = isam.getIndexSearcherByName();
				return collect__(isam, searcher, result, container, storage, begin, end);
			}
		}

		template<typename Container, typename It, typename Searcher>
		static void collect__(ISAM const &isam, Searcher const &searcher, Result<Protocol> &result, Container &container, std::string_view storage, It begin, It end){
			for(auto itk = begin; itk != end; ++itk)
				container.push_back( isam.load(storage.data(), searcher, *itk) );

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"imget",		"IMGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct ISETALL : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

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

			using MyISETALL_Factory = ISETALL_Factory<ParamContainer::iterator>;

			hm4::insertV<MyISETALL_Factory>(*db, key, isam, std::begin(p) + varg, std::end(p));

			return result.set_1();
		}


		template<typename It>
		struct ISETALL_Factory : hm4::PairFactory::IFactoryAction<1,1, ISETALL_Factory<It> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1, ISETALL_Factory<It> >;

			constexpr ISETALL_Factory(std::string_view const key, ISAM const &isam, It begin, It end) :
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

		private:
			ISAM const	&isam;
			It		it;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"isetall",		"ISETALL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct ISET : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

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

	private:
		template<typename It>
		static void process_1__(DBAdapter &db, Result<Protocol> &result,
				std::string_view const schema, std::string_view const key, It begin, It end){

			auto const &[isam, searcher] = ISAM::createAndSearchByName(schema, *begin);

			const auto *pair = hm4::getPairPtrWithSize(*db, key, isam.bytes());

			return process__(db, result, key, pair, isam, searcher, begin, end);
		}

		template<typename It>
		static void process_N__(DBAdapter &db, Result<Protocol> &result,
				std::string_view const schema, std::string_view const key, It begin, It end){
			ISAM const isam{ schema };

			const auto *pair = hm4::getPairPtrWithSize(*db, key, isam.bytes());

			if (isam.size() <= LINEAR_SEARCH_MAX){
				auto searcher = isam.getLinearSearcherByName();

				return process__(db, result, key, pair, isam, searcher, begin, end);
			}else{
				auto searcher = isam.getLinearSearcherByName();

				return process__(db, result, key, pair, isam, searcher, begin, end);
			}
		}

		template<typename Searcher, typename It>
		static void process__(DBAdapter &db, Result<Protocol> &result,
				std::string_view const key, const hm4::Pair *pair, ISAM const &isam, Searcher const &searcher, It begin, It end){

			using MyISET_Factory = ISET_Factory<Searcher, ParamContainer::iterator>;

			MyISET_Factory factory{ key, pair, isam, searcher, begin, end };

			insertHintVFactory(pair, *db, factory);

			return result.set(
				factory.getResult()
			);
		}

		template<typename Searcher, typename It>
		struct ISET_Factory : hm4::PairFactory::IFactoryAction<1,1, ISET_Factory<Searcher, It> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1, ISET_Factory<Searcher, It> >;

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

		private:
			ISAM const	&isam;
			Searcher const	&searcher;
			It		begin;
			It		end;
			bool		status		= false;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"iset",		"ISET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IDEL : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

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

	private:
		template<typename It>
		static void process_1__(DBAdapter &db, Result<Protocol> &result,
				std::string_view const schema, std::string_view const key, It begin, It end){

			auto const &[isam, searcher] = ISAM::createAndSearchByName(schema, *begin);

			const auto *pair = hm4::getPairPtrWithSize(*db, key, isam.bytes());

			return process__(db, result, key, pair, isam, searcher, begin, end);
		}

		template<typename It>
		static void process_N__(DBAdapter &db, Result<Protocol> &result,
				std::string_view const schema, std::string_view const key, It begin, It end){
			ISAM const isam{ schema };

			const auto *pair = hm4::getPairPtrWithSize(*db, key, isam.bytes());

			if (isam.size() <= LINEAR_SEARCH_MAX){
				auto searcher = isam.getLinearSearcherByName();

				return process__(db, result, key, pair, isam, searcher, begin, end);
			}else{
				auto searcher = isam.getLinearSearcherByName();

				return process__(db, result, key, pair, isam, searcher, begin, end);
			}
		}

		template<typename Searcher, typename It>
		static void process__(DBAdapter &db, Result<Protocol> &result,
				std::string_view const key, const hm4::Pair *pair, ISAM const &isam, Searcher const &searcher, It begin, It end){

			using MyIDEL_Factory = IDEL_Factory<Searcher, ParamContainer::iterator>;

			MyIDEL_Factory factory{ key, pair, isam, searcher, begin, end };

			insertHintVFactory(pair, *db, factory);

			return result.set(
				factory.getResult()
			);
		}

		template<typename Searcher, typename It>
		struct IDEL_Factory : hm4::PairFactory::IFactoryAction<1,1, IDEL_Factory<Searcher, It> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1, IDEL_Factory<Searcher, It> >;

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

		private:
			ISAM const	&isam;
			Searcher const	&searcher;
			It		begin;
			It		end;
			bool		status		= false;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"idel",		"IDEL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IRESERVE : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

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
		constexpr inline static std::string_view cmd[]	= {
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



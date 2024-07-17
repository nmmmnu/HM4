#include "base.h"
#include "mystring.h"

#include "hexconvert.h"

#include "shared_stoppredicate.h"
#include "shared_iterations.h"
#include "shared_zset_multi.h"

#include "ilist/txguard.h"

namespace net::worker::commands::LinearCurve{
	namespace linear_curve_impl_{

		using namespace net::worker::shared::stop_predicate;
		using namespace net::worker::shared::config;

		constexpr size_t scoreSize		=  8 * 2;	// uint64_t as hex
		using MC1Buffer = std::array<char, scoreSize>;

		constexpr std::string_view toHex(uint64_t const x, MC1Buffer &buffer){
			using namespace hex_convert;

			constexpr auto opt = options::lowercase | options::nonterminate;

			return hex_convert::toHex<uint64_t, opt>(x, buffer);
		}

		std::string_view toHex(std::string_view const x, MC1Buffer &buffer){
			return toHex(
				from_string<uint64_t>(x),
				buffer
			);
		}

		template<size_t N>
		std::string_view formatLine(uint64_t x, uint32_t id, std::array<char, N> &buffer){
			// largest uint64_t is 20 digits.
			static_assert(N > 20 + 1 + 8);

			constexpr static std::string_view fmt_mask = "{:0>20},{:08x}";

			auto const result = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask, x, id);

			if (result.out == std::end(buffer))
				return "PLS_REPORT_A_BUG";
			else
				return { buffer.data(), result.size };
		}

		using P1 = net::worker::shared::zsetmulti::Permutation1;

		constexpr bool isMC1KeyValid(std::string_view keyN, std::string_view keySub){
			return P1::valid(keyN, keySub, scoreSize);
		}

		template<class DBAdapter>
		void linearSearchPoint(
				DBAdapter &db,
				OutputBlob::Container &container, OutputBlob::BufferContainer &bcontainer,
				std::string_view keyN, uint32_t count,
				uint64_t x, std::string_view startKey){

			hm4::PairBufferKey bufferKeyPrefix;

			auto const prefix = [&](){
				if (!startKey.empty())
					return startKey;

				MC1Buffer buffer;

				return P1::makeKey(bufferKeyPrefix, DBAdapter::SEPARATOR,
						keyN,
						"A",
						toHex(x, buffer)
				);
			}();

			StopPrefixPredicate stop(prefix);

			uint32_t iterations	= 0;
			uint32_t results	= 0;
			uint32_t id		= 0;

			auto tail = [&](std::string_view const pkey = ""){
				container.emplace_back(pkey);
			};

			for(auto it = db->find(prefix, std::false_type{}); it != db->end(); ++it){
				auto const &key = it->getKey();

				if (!same_prefix(prefix, key))
					break;

				if (stop(key))
					return tail(); // no tail

				if (++iterations > ITERATIONS_LOOPS_MAX)
					return tail(key);

				if (! it->isOK())
					continue;

				// because of the prefix check in StopPrefixPredicate,
				// the point is always correct (inside).

				if (++results > count)
					return tail(key);

				auto const &val = it->getVal();

				bcontainer.push_back();
				auto const line = formatLine(x, id++, bcontainer.back());

				container.emplace_back(line);
				container.emplace_back(val);
			}

			return tail();
		}



		template<class DBAdapter>
		void linearSearch(
				DBAdapter &db, OutputBlob::Container &container, OutputBlob::BufferContainer &bcontainer,
				std::string_view keyN, uint32_t count,
				uint64_t x_min, uint64_t x_max, std::string_view startKey){

			auto createKey = [keyN](hm4::PairBufferKey &bufferKey, uint64_t x){
				MC1Buffer buffer;
				return P1::makeKey(bufferKey, DBAdapter::SEPARATOR,
						keyN,
						"A",
						toHex(x, buffer)
				);
			};

			hm4::PairBufferKey bufferKey[2];

			auto       key_min = 	! startKey.empty() ? startKey :
						createKey(bufferKey[0], x_min );
			auto const key_max =	createKey(bufferKey[1], x_max );

			StopRangePrefixPredicate stop(key_max);

			uint32_t iterations	= 0;
			uint32_t results	= 0;
			uint32_t id		= 0;

			auto tail = [&](std::string_view const pkey = ""){
				container.emplace_back(pkey);
			};

			for(auto it = db->find(key_min, std::false_type{}); it != db->end(); ++it){
				auto const &key = it->getKey();

				if (stop(key))
					return tail(); // no tail

				if (++iterations > ITERATIONS_LOOPS_MAX)
					return tail(key);

				if (! it->isOK())
					continue;

				auto const hexA = P1::decodeIndex(DBAdapter::SEPARATOR,
							after_prefix(shared::zsetmulti::prefixSize(keyN, "A"), key));

				auto const hex  = hexA[0];

				auto const x    = hex_convert::fromHex<uint64_t>(hex);

				// because of the prefix check in StopPrefixPredicate,
				// the point is always correct (inside).

				if (++results > count)
					return tail(key);

				auto const &val = it->getVal();

				bcontainer.push_back();
				auto const line = formatLine(x, id++, bcontainer.back());

				container.emplace_back(line);
				container.emplace_back(val);
			}

			return tail();
		}

	} // namespace linear_curve_impl_



	template<class Protocol, class DBAdapter>
	struct MC1GET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC1GET key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace linear_curve_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (keyN.empty() || keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC1KeyValid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			return result.set(
				shared::zsetmulti::get<P1>(db, keyN, keySub)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc1get",	"MC1GET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC1MGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC1GET key subkey0 subkey1 ...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace linear_curve_impl_;

			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_3);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &keySub = *itk;

				if (keySub.empty())
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				if (!isMC1KeyValid(keyN, keySub))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}



			auto &container = blob.container();



			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &keySub = *itk;

				container.emplace_back(
					shared::zsetmulti::get<P1>(db, keyN, keySub)
				);
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc1mget",	"MC1MGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC1EXISTS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC1GET key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace linear_curve_impl_;

			return shared::zsetmulti::cmdProcessExists<P1>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc1exists",	"MC1EXISTS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC1SCORE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC1SCORE key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace linear_curve_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (keyN.empty() || keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC1KeyValid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			return result.set_container(
				shared::zsetmulti::getIndexes<P1>(db, keyN, keySub)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc1score",	"MC1SCORE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC1ADD : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC1ADD a keySub0 x0 val0 keySub1 x1 val1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace linear_curve_impl_;

			auto const varg  = 2;
			auto const vstep = 3;

			if (p.size() < varg + vstep || (p.size() - varg) % vstep != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_4);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const &keySub	= *(itk + 0);
			//	auto const &x		= *(itk + 1);
				auto const &value	= *(itk + 2);

				if (!isMC1KeyValid(keyN, keySub))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

				if (!hm4::Pair::isValValid(value))
					return result.set_error(ResultErrorMessages::EMPTY_VAL);
			}

			hm4::TXGuard guard{ *db };

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const &keySub	= *(itk + 0);
				auto const &x		= *(itk + 1);
				auto const &value	= *(itk + 2);

				MC1Buffer buffer;

				auto const score	= toHex(x, buffer);

				shared::zsetmulti::add<P1>(
						db,
						keyN, keySub, { score }, value
				);
			}

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc1add",		"MC1ADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC1REM : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC1DEL a subkey0 subkey1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace linear_curve_impl_;

			hm4::TXGuard guard{ *db };

			return shared::zsetmulti::cmdProcessRem<P1>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc1rem",		"MC1REM"	,
			"mc1remove",		"MC1REMOVE"	,
			"mc1del",		"MC1DEL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC1POINT : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC1POINT linear 10 10000 [key]

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace linear_curve_impl_;

			if (p.size() != 4 && p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_45);

			auto const keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC1KeyValid(keyN, "x"))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const x		= from_string<uint32_t>(p[2]);

			auto const count	= myClamp<uint32_t>(p[3], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);

			auto const startKey	= p.size() == 5 ? p[4] : "";

			auto &container = blob.container();
			auto &bcontainer = blob.bcontainer();

			linearSearchPoint(
				db,
				container, bcontainer,
				keyN, count,
				x,
				startKey
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc1point",	"MC1POINT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC1RANGE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC1RANGENAIVE linear 10 10 10000 [key]

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace linear_curve_impl_;

			if (p.size() != 5 && p.size() != 6)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_67);

			auto const keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC1KeyValid(keyN, "x"))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const x_min	= from_string<uint32_t>(p[2]);
			auto const x_max	= from_string<uint32_t>(p[3]);

			auto const count	= myClamp<uint32_t>(p[4], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);

			auto const startKey	= p.size() == 6 ? p[5] : "";

			auto &container = blob.container();
			auto &bcontainer = blob.bcontainer();

			linearSearch(
				db, container, bcontainer,
				keyN, count,
				x_min, x_max,
				startKey
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc1range",	"MC1RANGE"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "linear_curve";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				MC1GET			,
				MC1MGET			,
				MC1EXISTS		,
				MC1SCORE		,
				MC1ADD			,
				MC1REM			,
				MC1POINT		,
				MC1RANGE
			>(pack);
		}
	};



} // namespace


#include "base.h"
#include "mystring.h"

#include "mortoncurve.h"
#include "hexconvert.h"

#include "shared_stoppredicate.h"
#include "shared_iterations.h"

namespace net::worker::commands::MortonCurve{
	namespace morton_curve_impl_{

		using namespace net::worker::shared::stop_predicate;
		using namespace net::worker::shared::config;

		namespace {
			using MC2Buffer = to_string_buffer_t;

			constexpr size_t subNSize		=  8 * 2 + 1;	// uint64_t as hex + '\0'
		//	constexpr size_t decryptedKeySize	= 10 * 2 + 2;	// 2 x uint32_t as dec + ',' + '\0'

			static_assert(
				to_string_buffer_t_size >= subNSize
			//	&&
			//	to_string_buffer_t_size >= decryptedKeySize
			);

			constexpr bool isMC2KeyValid(std::string_view key){
				return hm4::Pair::isKeyValid(key.size() + subNSize + 1 + 16);
			}

			constexpr std::string_view toHex(uint64_t const z, MC2Buffer &buffer){
				using namespace hex_convert;

				constexpr auto opt = options::lowercase | options::nonterminate;

				return hex_convert::toHex<uint64_t, opt>(z, buffer.data());
			}

			constexpr std::string_view toHex(uint32_t const x, uint32_t const y, MC2Buffer &buffer){
				return toHex(
					morton_curve::toMorton2D(x, y),
					buffer
				);
			}

			std::string_view toHex(std::string_view const x, std::string_view const y, MC2Buffer &buffer){
				return toHex(
					from_string<uint32_t>(x),
					from_string<uint32_t>(y),
					buffer
				);
			}



			struct MortonSquare{
				uint32_t x1;
				uint32_t x2;
				uint32_t y1;
				uint32_t y2;

				uint64_t z_min = morton_curve::toMorton2D(x1, y1);
				uint64_t z_max = morton_curve::toMorton2D(x2, y2);

				constexpr MortonSquare(uint32_t x1, uint32_t x2, uint32_t y1, uint32_t y2) :
						x1(x1), x2(x2),
						y1(y1), y2(y2){}

				constexpr bool inside(uint32_t x, uint32_t y) const{
					return x >= x1 && x <= x2 && y >= y1 && y <= y2;
				}

				auto bigmin(uint64_t z) const{
					return morton_curve::computeBigMinFromMorton2D(z, z_min, z_max);
				}

				void print() const{
					logger<Logger::DEBUG>() << x1 << x2 << y1 << y2 << z_min << z_max;
				}
			};



			#ifdef __GNUG__
				// this silence the warning for unused label.
				// see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=113582
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wunused-label"
			#endif

			template<bool bigmin_optimized, class DBAdapter, class BufferKeyArray>
			void mortonSearch(
					DBAdapter &db, OutputBlob::Container &container,
					BufferKeyArray &bufferKey,
					std::string_view keyN, uint32_t count,
					MortonSquare const &square, std::string_view startKey){

				constexpr uint32_t MAX_RETRIES = 9;

				auto projKey = [prefix_size = keyN.size()](std::string_view x) -> std::string_view{
					if (prefix_size <= x.size())
						return x.substr(prefix_size + 1);
					else
						return x;
				};

				auto createKey = [keyN](OutputBlob::BufferKey &bufferKey, uint64_t z){
					to_string_buffer_t z_buffer;
					return concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR, toHex(z, z_buffer));
				};

				auto       key_min = 	! startKey.empty() ? startKey :
							createKey(bufferKey[0], square.z_min );
				auto const key_max =	createKey(bufferKey[1], square.z_max );

				StopRangePredicate stop(key_max);

			//	square.print();

				uint32_t iterations	= 0;
				uint32_t results	= 0;
				uint32_t skips		= 0;
				uint32_t retries	= 0;

				auto tail = [&](std::string_view const pkey = ""){
					logger<Logger::DEBUG>() << "MORTON SEARCH DONE >>>"
						<< "iterations"	<< iterations
						<< "results"	<< results
						<< "retries"	<< retries
					;

					container.emplace_back(pkey);
				};

			start: // label for goto :)

				for(auto it = db->find(key_min, std::false_type{}); it != db->end(); ++it){
					auto const &key = it->getKey();

					if (++iterations > ITERATIONS_LOOPS_MAX)
						return tail(key);

					if (stop(key))
						return tail(); // no tail

					if (! it->isOK())
						continue;

					auto const hex = projKey(key);

					auto const z = hex_convert::fromHex<uint64_t>(hex);

					auto const [x, y] = morton_curve::fromMorton2D(z);

					if (square.inside(x, y)){
						if (++results > count)
							return tail(key);

						auto const &val = it->getVal();

						container.emplace_back(hex);

						container.emplace_back(val);

						if constexpr(bigmin_optimized){
							skips = 0;
						}

					//	logger<Logger::DEBUG>() << "Y >>>" << hexKey << x << y;
					}else{
						if constexpr(bigmin_optimized){
							if (++skips > ITERATIONS_IDLE){
								if (++retries > MAX_RETRIES)
									return tail(key);

								auto const big_min = square.bigmin(z);

								// done or error
								if (big_min > square.z_max)
									return tail();

								key_min = createKey(bufferKey[0], big_min);

								// unfortunately the it.operator= is deleted.
								goto start;
							}
						}

					//	logger<Logger::DEBUG>() << "N >>>" << hexKey << x << y;
					}
				}

				return tail();
			}

			#ifdef __GNUG__
				// this silence the warning for unused label.
				// see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=113582
				#pragma GCC diagnostic pop
			#endif

		} // namespace

	} // namespacemorton_curve_impl_



	template<class Protocol, class DBAdapter>
	struct MC2GET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC2GET key x y

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC2KeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const &x = p[2];
			auto const &y = p[3];

			MC2Buffer buffer;

			auto const subN = toHex(x, y, buffer);

			auto const key = concatenateBuffer(blob.buffer_key[0], keyN, DBAdapter::SEPARATOR, subN);

			return result.set(
				hm4::getPairVal(*db, key)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2get",	"MC2GET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2MGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC2GET key x0 y0 x1 y1 x2 y2

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			if (p.size() < 4 && p.size() % 2 == 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_4);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC2KeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto &container = blob.container;
			container.clear();

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &x = *itk;
				auto const &y = *std::next(itk);

				MC2Buffer buffer;

				auto const subN = toHex(x, y, buffer);

				auto const key = concatenateBuffer(blob.buffer_key[0], keyN, DBAdapter::SEPARATOR, subN);

				container.emplace_back(
					hm4::getPairVal(*db, key)
				);
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2mget",	"MC2MGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2EXISTS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC2KeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const &x = p[2];
			auto const &y = p[3];

			MC2Buffer buffer;

			auto const subN = toHex(x, y, buffer);

			auto const key = concatenateBuffer(blob.buffer_key[0], keyN, DBAdapter::SEPARATOR, subN);

			return result.set(
				hm4::getPairOK(*db, key)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2exists",	"MC2EXISTS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2SET : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC2SET a x0 y0 val0 x1 y1 val1

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			if (p.size() < 4 || (p.size() - 2) % 3 != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_4);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC2KeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 3){
				if (auto const &val = *(itk + 2); !hm4::Pair::isValValid(val))
					return result.set_error(ResultErrorMessages::EMPTY_VAL);
			}

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 3){
				auto const &x = *itk;
				auto const &y = *std::next(itk);

				MC2Buffer buffer;

				auto const subN = toHex(x, y, buffer);

				auto const key = concatenateBuffer(blob.buffer_key[0], keyN, DBAdapter::SEPARATOR, subN);

				auto const &val = *(itk + 2);

				hm4::insert(*db, key, val);
			}

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2set",		"MC2SET"	,
			"mc2mset",		"MC2MSET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2DEL : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC2DEL a x0 y0 x1 y1

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			if (p.size() < 4 && p.size() % 2 == 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_4);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC2KeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &x = *itk;
				auto const &y = *std::next(itk);

				MC2Buffer buffer;

				auto const subN = toHex(x, y, buffer);

				auto const key = concatenateBuffer(blob.buffer_key[0], keyN, DBAdapter::SEPARATOR, subN);

				hm4::erase(*db, key);
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2del",		"MC2DEL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2XNGETNAIVE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			if (p.size() != 7 && p.size() != 8)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_67);

			auto const keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC2KeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			MortonSquare const square{
				from_string<uint32_t>(p[2]),
				from_string<uint32_t>(p[3]),

				from_string<uint32_t>(p[4]),
				from_string<uint32_t>(p[5])
			};

			auto const count	= myClamp<uint32_t>(p[6], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);

			auto const startKey	= p.size() == 8 ? p[7] : "";

			blob.container.clear();

			mortonSearch<false>(
				db, blob.container, blob.buffer_key,
				keyN, count,
				square, startKey
			);

			return result.set_container(blob.container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2xngetnaive",	"MC2XNGETNAIVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2XNGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			if (p.size() != 7 && p.size() != 8)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_67);

			auto const keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC2KeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			MortonSquare const square{
				from_string<uint32_t>(p[2]),
				from_string<uint32_t>(p[3]),

				from_string<uint32_t>(p[4]),
				from_string<uint32_t>(p[5])
			};

			auto const count	= myClamp<uint32_t>(p[6], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);

			auto const startKey	= p.size() == 8 ? p[7] : "";

			blob.container.clear();

			mortonSearch<true>(
				db, blob.container, blob.buffer_key,
				keyN, count,
				square, startKey
			);

			return result.set_container(blob.container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2xnget",	"MC2XNGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2ENCODE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			using namespace morton_curve_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &x = p[1];
			auto const &y = p[2];

			MC2Buffer buffer;

			auto const hex = toHex(x, y, buffer);

			return result.set(hex);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2encode",	"MC2ENCODE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2DECODE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			using namespace morton_curve_impl_;

			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			auto const &hex = p[1];

			auto const z = hex_convert::fromHex<uint64_t>(hex);

			auto const [x, y] = morton_curve::fromMorton2D(z);

			MC2Buffer buffer[2];

			const std::array<std::string_view, 2> container{
				to_string(x, buffer[0]),
				to_string(y, buffer[2])
			};

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2decode",	"MC2DECODE"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "morton_curve";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				MC2GET		,
				MC2MGET		,
				MC2EXISTS	,
				MC2SET		,
				MC2DEL		,
				MC2XNGETNAIVE	,
				MC2XNGET	,
				MC2ENCODE	,
				MC2DECODE
			>(pack);
		}
	};



} // namespace


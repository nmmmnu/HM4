#include "base.h"
#include "mystring.h"

#include "mortoncurve.h"
#include "hexconvert.h"

namespace net::worker::commands::MortonCurve{
	namespace morton_curve_impl_{
		using MC2Buffer = to_string_buffer_t;

		constexpr size_t subNSize		= 8 * 2 + 1;	// uint64_t as hex + '\0'
		constexpr size_t decryptedKeySize	= 10 + 2;	// 2 x uint32_t as dec + ',' + '\0'

		static_assert(
			to_string_buffer_t_size >= subNSize &&
			to_string_buffer_t_size >= decryptedKeySize
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

		inline std::string_view toHex(std::string_view const x, std::string_view const y, MC2Buffer &buffer){
			return toHex(
				from_string<uint32_t>(x),
				from_string<uint32_t>(y),
				buffer
			);
		}
	}



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

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);

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

				auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);

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

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);

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

				auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);

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

				auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);

				hm4::erase(*db, key);
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2del",		"MC2DEL"
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
				MC2DEL
			>(pack);
		}
	};



} // namespace


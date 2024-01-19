#include "base.h"
#include "mystring.h"

#include "mortoncurve.h"
#include "hexconvert.h"

namespace net::worker::commands::MortonCurve{
	namespace morton_curve_impl_{
		constexpr size_t subNSize = 8 * 2; // uint64_t as hex

		using MC2Buffer = std::array<char, subNSize>;

		constexpr bool isMC2KeyValid(size_t keyN){
			return hm4::Pair::isKeyValid(keyN + subNSize + 1 + 16);
		}

		constexpr bool isMC2KeyValid(std::string_view key){
			return isMC2KeyValid(key.size());
		}

		constexpr std::string_view toHex(uint64_t const x, MC2Buffer &buffer){
			using namespace hex_convert;

			constexpr auto opt = options::lowercase | options::nonterminate;

			return hex_convert::toHex<uint64_t, opt>(x, buffer.data());
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

			auto const x = from_string<uint32_t>(p[2]);
			auto const y = from_string<uint32_t>(p[3]);

			auto const z = morton_curve::toMorton2D(x, y);

			MC2Buffer buffer;

			auto const subN = toHex(z, buffer);

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);

			logger<Logger::DEBUG>() << "Morton" << x << y << z << subN << key;

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

			auto &container = blob.container;
			container.clear();

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const x = from_string<uint32_t>(*itk);
				auto const y = from_string<uint32_t>(*std::next(itk));

				auto const z = morton_curve::toMorton2D(x, y);

				MC2Buffer buffer;

				auto const subN = toHex(z, buffer);

				auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);

				logger<Logger::DEBUG>() << "Morton" << x << y << z << subN << key;

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

			auto const x = from_string<uint32_t>(p[2]);
			auto const y = from_string<uint32_t>(p[3]);

			auto const z = morton_curve::toMorton2D(x, y);

			MC2Buffer buffer;

			auto const subN = toHex(z, buffer);

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);

			logger<Logger::DEBUG>() << "Morton" << x << y << z << subN << key;

			return result.set(
				hm4::getPairOK(*db, key)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2exists",	"MC2EXISTS"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "immutable";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				MC2GET		,
				MC2MGET		,
				MC2EXISTS	/*,
				MC2SET		,
				MC2DEL	*/
			>(pack);
		}
	};



} // namespace


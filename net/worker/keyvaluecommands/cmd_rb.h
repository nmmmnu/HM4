#include "base.h"
#include "ringbuffer.h"
#include "logger.h"
#include "pair_vfactory.h"
#include "shared_hint.h"

namespace net::worker::commands::RB{
	namespace rb_impl_{
		constexpr uint16_t MIN_SLOTS	=    1;
		constexpr uint16_t MAX_SLOTS	= 4096;

		constexpr bool isSlotsValid(uint16_t slots){
			return slots >= MIN_SLOTS && slots <= MAX_SLOTS;
		}

		template<typename T>
		struct type_identity{
			// C++20 std::type_identity
			using type = T;
		};

		template<typename F>
		auto type_dispatch(size_t const t, F f){
			using namespace ring_buffer;

			switch(t){
			case   16 : return f(type_identity<RawRingBuffer16	>{});
			case   32 : return f(type_identity<RawRingBuffer32	>{});
			case   40 : return f(type_identity<RawRingBuffer40	>{});
			case   64 : return f(type_identity<RawRingBuffer64	>{});
			case  128 : return f(type_identity<RawRingBuffer128	>{});
			case  256 : return f(type_identity<RawRingBuffer256	>{});
			case  512 : return f(type_identity<RawRingBuffer512	>{});
			case 1024 : return f(type_identity<RawRingBuffer1024	>{});
			default   : return f(type_identity<std::nullptr_t	>{});
			}
		}

	} // namespace rb_impl_



	template<class Protocol, class DBAdapter>
	struct RBRESERVE : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// RBRESERVE key slots bytes

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace rb_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint16_t	>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const bytes = from_string<size_t	>(p[3]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					using MyRingBuffer = T;

					MyRingBuffer const rb{ slots };

					hm4::insertV<hm4::PairFactory::Reserve>(*db, key, rb.bytes());

					return result.set_1();
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"rbreserve",	"RBRESERVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct RBADD : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// RBADD key slots bytes item item

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace rb_impl_;

			if (p.size() < 5)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_5);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint16_t>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const bytes = from_string<size_t	>(p[3]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					using MyRingBuffer = T;

					MyRingBuffer const rb{ slots };

					return process_(rb, key, p, *db, result);
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		template<typename MyRingBuffer>
		void process_(MyRingBuffer const &rb, std::string_view const key, ParamContainer const &p, typename DBAdapter::List &list, Result<Protocol> &result) const{
			auto const varg = 4;
			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &item = *itk; !rb.isItemValid(item))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto *pair = hm4::getPairPtrWithSize(list, key, rb.bytes());

			using MyRBADDFactory = RBADDFactory<MyRingBuffer>;

			MyRBADDFactory factory{ key, pair, rb, std::begin(p) + varg, std::end(p) };

			insertHintVFactory(list, pair, factory);

			return result.set_1();
		}

	private:
		template<typename MyRingBuffer>
		struct RBADDFactory : hm4::PairFactory::IFactoryAction<1,1,RBADDFactory<MyRingBuffer> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,RBADDFactory>;

			using It   = ParamContainer::const_iterator;

			RBADDFactory(std::string_view const key, const Pair *pair, MyRingBuffer rb, It begin, It end) :
							Base::IFactoryAction	(key, rb.bytes(), pair),
							rb			(rb	),
							begin			(begin	),
							end			(end	){}

			void action(Pair *pair){
				using List = typename MyRingBuffer::List;

				auto *rb_data = hm4::getValAs<List>(pair);

				for(auto itk = begin; itk != end; ++itk){
					auto const &item = *itk;

					rb.push(*rb_data, item);
				}
			}

		private:
			MyRingBuffer	rb;
			It		begin;
			It		end;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"rbadd",	"RBADD"	,
			"rbpush",	"RBpUSH"
		};
	};



	template<class Protocol, class DBAdapter>
	struct RBPOP : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// RBPOP key slots bytes

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace rb_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint16_t>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const bytes = from_string<size_t	>(p[3]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					using MyRingBuffer = T;

					MyRingBuffer const rb{ slots };

					return process_(rb, key, *db, result);
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		template<typename MyRingBuffer>
		void process_(MyRingBuffer const &rb, std::string_view const key, typename DBAdapter::List &list, Result<Protocol> &result) const{
			const auto *pair = hm4::getPairPtrWithSize(list, key, rb.bytes());

			using MyRBADDFactory = RBPOPFactory<MyRingBuffer>;

			MyRBADDFactory factory{ key, pair, rb };

			insertHintVFactory(list, pair, factory);

			return result.set(
				factory.getResult()
			);
		}

	private:
		template<typename MyRingBuffer>
		struct RBPOPFactory : hm4::PairFactory::IFactoryAction<1,1,RBPOPFactory<MyRingBuffer> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,RBPOPFactory>;

			using It   = ParamContainer::const_iterator;

			RBPOPFactory(std::string_view const key, const Pair *pair, MyRingBuffer rb) :
							Base::IFactoryAction	(key, rb.bytes(), pair),
							rb			(rb	){}

			void action(Pair *pair){
				using List = typename MyRingBuffer::List;

				auto *rb_data = hm4::getValAs<List>(pair);

				result = rb.pop(*rb_data);
			}

			constexpr auto getResult() const{
				return result;
			}

		private:
			MyRingBuffer		rb;
			std::string_view	result;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"rbpop",	"RBPOP"
		};
	};



	template<class Protocol, class DBAdapter>
	struct RBGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// RBGET key slots bytes

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace rb_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint16_t	>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const bytes = from_string<size_t	>(p[3]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					using MyRingBuffer = T;

					MyRingBuffer const rb{ slots };

					const auto *pair = hm4::getPairPtrWithSize(*db, key, rb.bytes());

					return process_(rb, pair, result, blob);
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		template<class MyRingBuffer>
		void process_(MyRingBuffer const &rb, const hm4::Pair *pair, Result<Protocol> &result, OutputBlob &blob) const{
			auto &container = blob.construct<OutputBlob::Container>();

			if (pair == nullptr)
				return result.set_container(container);

			using List = typename MyRingBuffer::List;

			const auto *rb_data = hm4::getValAs<List>(pair);

			auto f = [&container](std::string_view sv){
				container.push_back(sv);
			};

			rb.for_each(*rb_data, f);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"rbget",	"RBGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct RBCOUNT : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// RBGETCOUNT key slots bytes

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace rb_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint16_t	>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const bytes = from_string<size_t	>(p[3]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					using MyRingBuffer = T;

					MyRingBuffer const rb{ slots };

					const auto *pair = hm4::getPairPtrWithSize(*db, key, rb.bytes());

					return process_(rb, pair, result);
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		template<class MyRingBuffer>
		void process_(MyRingBuffer const &rb, const hm4::Pair *pair, Result<Protocol> &result) const{
			if (pair == nullptr)
				return result.set_0();

			using List = typename MyRingBuffer::List;

			const auto *rb_data = hm4::getValAs<List>(pair);

			return result.set(rb.count(*rb_data));
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"rbcount",	"RBCOUNT",
			"rblen",	"RBLEN"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "rb";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				RBRESERVE	,
				RBADD		,
				RBPOP		,
				RBGET		,
				RBCOUNT
			>(pack);
		}
	};



} // namespace


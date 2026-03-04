#ifndef MY_HASHTABLE_H_
#define MY_HASHTABLE_H_

#include <utility>

#include <cstddef>

namespace myhashtable{

	namespace impl_{
		enum class LocateType{
			FOUND	,
			EMPTY	,
			ERROR
		};

		struct Locator{
			LocateType	type;
			size_t		id;
		};

		template<typename Controller>
		[[nodiscard]]
		constexpr Locator locate_(Controller const &controller, typename Controller::key_type const &key){
			static_assert(Controller::size() > 0, "Size must be positive number");

			static_assert(
				(Controller::size() & (Controller::size() - 1)) == 0,
				"Controller::size() must be power of two"
			);

			size_t const mask = controller.size() - 1;
			size_t const cell = controller.hash(key) & mask;

			for (size_t i = 0; i < controller.size(); ++i){
				size_t const id = (cell + i) & mask;

				if (controller.getStorage()(id))
					return { LocateType::EMPTY, id };

				if (controller.equal(id, key))
					return { LocateType::FOUND, id };
			}

			return { LocateType::ERROR, 0 };
		}

		template<typename Controller, typename... Ts>
		[[nodiscard]]
		constexpr bool insertT_(Controller &controller, typename Controller::key_type const &key, Ts &&...ts){
			using namespace impl_;

			auto [type, id] = locate_(controller, key);

			if (type == LocateType::ERROR)
				return false;

			controller.getStorage().emplace(id, key, std::forward<Ts>(ts)...);
			return true;
		}

		template<typename Controller, typename u_value_type>
		[[nodiscard]]
		constexpr bool insertF_(Controller &controller, u_value_type &&data){
			auto const &key = controller.getKey(data);

			using namespace impl_;

			auto [type, id] = locate_(controller, key);

			if (type == LocateType::ERROR)
				return false;

			controller.getStorage().emplace(id, std::forward<u_value_type>(data));
			return true;
		}

	} // namespace impl_



	template<typename Controller, typename... Ts>
	constexpr bool insert(Controller &controller, typename Controller::key_type const &key, Ts &&...ts){
		return impl_::insertT_(controller, key, std::forward<Ts>(ts)...);
	}

	template<typename Controller>
	constexpr bool insert(Controller &controller, typename Controller::value_type const &data){
		return impl_::insertF_(controller, data);
	}

	template<typename Controller>
	constexpr bool insert(Controller &controller, typename Controller::value_type &&data){
		return impl_::insertF_(controller, std::move(data));
	}

	template<typename Controller>
	[[nodiscard]]
	constexpr const typename Controller::mapped_type *find(Controller const &controller, typename Controller::key_type const &key){
		using namespace impl_;

		auto [type, id] = locate_(controller, key);

		if (type != LocateType::FOUND)
			return nullptr;

		return & controller.getVal(controller.getStorage()[id]);
	}

	template<typename Controller>
	[[nodiscard]]
	constexpr typename Controller::mapped_type *findMut(Controller &controller, typename Controller::key_type const &key){
		using namespace impl_;

		auto [type, id] = locate_(controller, key);

		if (type != LocateType::FOUND)
			return nullptr;

		return & controller.getVal(controller.getStorage()[id]);
	}

	template<typename Controller>
	[[nodiscard]]
	constexpr bool exists(Controller const &controller, typename Controller::key_type const &key){
		using namespace impl_;

		auto [type, id] = locate_(controller, key);

		return type == LocateType::FOUND;
	}

	template<typename Controller>
	[[nodiscard]]
	constexpr size_t longestChain(Controller const &controller){
		size_t longest = 0;
		size_t chain   = 0;

		for(size_t i = 0; i < controller.size(); ++i){
			if (controller.getStorage()(i)){
				if (chain > longest)
					longest = chain;

				chain = 0;

				continue;
			}

			++chain;
		}

		return chain > longest ? chain : longest;
	}

} // namespace myhashtable

#endif



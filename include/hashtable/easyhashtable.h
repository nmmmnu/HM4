#ifndef MY_HASHTABLE_EASY_HASHTABLE_H_
#define MY_HASHTABLE_EASY_HASHTABLE_H_

#include <utility>	// std::forward

#include <cstddef>

namespace myhashtable{

	template<typename Controller, typename Storage>
	struct EasyHashtable{
		using key_type		= typename Controller::key_type		; // K
		using mapped_type	= typename Controller::mapped_type	; // V    or K
		using value_type	= typename Controller::value_type	; // pair or K

	public:
		template<typename... Ts>
		constexpr EasyHashtable(Ts &&...ts) : storage_(std::forward<Ts>(ts)...){}

	public:
		template<typename... Ts>
		constexpr bool insert(key_type const &key, Ts &&...ts){
			return insertT_(key, std::forward<Ts>(ts)...);
		}

		constexpr bool insert(value_type const &data){
			return insertF_(data);
		}

		constexpr bool insert(value_type &&data){
			return insertF_(std::move(data));
		}

	public:
		constexpr const mapped_type *find(key_type const &key) const{
			auto [type, id] = locate_(key);

			if (type != LocateType::FOUND)
				return nullptr;

			return & Controller::getVal(storage_[id]);
		}

		constexpr mapped_type *find(key_type const &key){
			auto [type, id] = locate_(key);

			if (type != LocateType::FOUND)
				return nullptr;

			return & Controller::getVal(storage_[id]);
		}

		constexpr bool exists(key_type const &key) const{
			auto [type, id] = locate_(key);

			return type == LocateType::FOUND;
		}

	public:
		constexpr void stats() const{
			return storage_.stats();
		}

		constexpr size_t longestChain() const{
			size_t longest = 0;
			size_t chain   = 0;

			for(size_t i = 0; i < storage_.size(); ++i){
				if (storage_(i)){
					if (chain > longest)
						longest = chain;

					chain = 0;

					continue;
				}

				++chain;
			}

			return chain > longest ? chain : longest;
		}

	private:
		constexpr bool equals_(size_t id, key_type const &key) const{
			return Controller::getKey(storage_[id]) == key;
		}

		enum class LocateType{
			FOUND	,
			EMPTY	,
			ERROR
		};

		struct Locator{
			LocateType	type;
			size_t		id;
		};

		constexpr Locator locate_(key_type const &key) const{
			size_t const mask = storage_.size() - 1;
			size_t const cell = Controller::hash(key) & mask;

			for (size_t i = 0; i < storage_.size(); ++i){
				size_t const id = (cell + i) & mask;

				if (storage_(id))
					return { LocateType::EMPTY, id };

				if (equals_(id, key))
					return { LocateType::FOUND, id };
			}

			return { LocateType::ERROR, 0 };
		}

	private:
		template<typename... Ts>
		constexpr bool insertT_(key_type const &key, Ts &&...ts){
			auto [type, id] = locate_(key);

			if (type == LocateType::ERROR)
				return false;

			storage_.emplace(id, key, std::forward<Ts>(ts)...);

			return true;
		}

		template<typename u_value_type>
		constexpr bool insertF_(u_value_type &&data){
			auto const &key = Controller::getKey(data);

			auto [type, id] = locate_(key);

			if (type == LocateType::ERROR)
				return false;

			storage_.emplace(id, std::forward<u_value_type>(data));

			return true;
		}

	private:
		Storage		storage_;
	};

} // namespace myhashtable

#endif


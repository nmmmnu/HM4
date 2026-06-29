#ifndef MY_HASHTABLE_EASY_HASHTABLE_H_
#define MY_HASHTABLE_EASY_HASHTABLE_H_

#include <utility>	// std::forward
#include <functional>	// std::hash

#include <cstddef>

namespace myhashtable{

	template<typename Adapter, typename Index, typename Storage, typename Hash = std::hash<typename Adapter::key_type> >
	struct EasyHashtable{
		using key_type		= typename Adapter::key_type	; // K
		using mapped_type	= typename Adapter::mapped_type	; // V    or K
		using value_type	= typename Adapter::value_type	; // pair or K

	private:
		constexpr static bool UseIndex = !std::is_same_v<Index, std::nullptr_t>;

	static_assert(
		(Storage::size() & (Storage::size() - 1)) == 0,
		"Storage::size() must be power of two"
	);

	public:
		template<typename... Ts>
		constexpr EasyHashtable(Ts &&...ts) :
						index_(),
						storage_(std::forward<Ts>(ts)...){}

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
		[[nodiscard]]
		constexpr const mapped_type *find(key_type const &key) const{
			auto [type, id, _] = locate_(key);

			if (type != Locator::FOUND)
				return nullptr;

			return & Adapter::getVal(storage_[id]);
		}

		[[nodiscard]]
		constexpr mapped_type *find(key_type const &key){
			auto [type, id, _] = locate_(key);

			if (type != Locator::FOUND)
				return nullptr;

			return & Adapter::getVal(storage_[id]);
		}

		[[nodiscard]]
		constexpr bool exists(key_type const &key) const{
			auto [type, id, _] = locate_(key);

			return type == Locator::FOUND;
		}

	public:
		constexpr void stats() const{
			return storage_.stats();
		}

		[[nodiscard]]
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
		[[nodiscard]]
		constexpr bool equals_(size_t id, key_type const &key) const{
			return Adapter::getKey(storage_[id]) == key;
		}



		struct Locator{
			constexpr static int ERROR = 0;
			constexpr static int EMPTY = 1;
			constexpr static int FOUND = 2;
		};

		template<typename Token>
		struct TokenLocator{
			int	result		;
			size_t	id	{}	;
			Token	token	{}	;
		};

		[[nodiscard]]
		constexpr auto locate_(key_type const &key) const{
			auto const mask = storage_.size() - 1;
			auto const hash = Hash{}(key);
			auto const cell = hash & mask;

			if constexpr(!UseIndex){
				// Standard

				using MyLocator = TokenLocator<std::nullptr_t>;

				for (size_t i = 0; i < storage_.size(); ++i){
					size_t const id = (cell + i) & mask;

					if (storage_(id))
						return MyLocator{ Locator::EMPTY, id };

					if (equals_(id, key))
						return MyLocator{ Locator::FOUND, id };
				}

				return MyLocator{ Locator::ERROR };

			}else{
				// TopHash or similar

				using MyLocator = TokenLocator<typename Index::token_type>;

				auto const token = index_.getToken(hash, key);

				for (size_t i = 0; i < storage_.size(); ++i){
					size_t const id = (cell + i) & mask;

					auto const ch = index_.check(id, token);

					if (ch == index_.EMPTY)
						return MyLocator{ Locator::EMPTY, id, token };

					if (ch == index_.FOUND && equals_(id, key))
						return MyLocator{ Locator::FOUND, id, token };
				}

				return MyLocator{ Locator::ERROR };
			}
		}

	private:
		template<typename... Ts>
		[[nodiscard]]
		constexpr bool insertT_(key_type const &key, Ts &&...ts){
			auto [type, id, token] = locate_(key);

			if (type == Locator::ERROR)
				return false;

			storage_.emplace(id, key, std::forward<Ts>(ts)...);

			if constexpr(UseIndex)
				index_.emplace(id, token);

			return true;
		}

		template<typename u_value_type>
		[[nodiscard]]
		constexpr bool insertF_(u_value_type &&data){
			auto const &key = Adapter::getKey(data);

			auto [type, id, token] = locate_(key);

			if (type == Locator::ERROR)
				return false;

			storage_.emplace(id, std::forward<u_value_type>(data));

			if constexpr(UseIndex)
				index_.emplace(id, token);

			return true;
		}

	private:
		Index		index_;
		Storage		storage_;
	};

} // namespace myhashtable

#endif


#ifndef MY_HASHTABLE_MAP_H_
#define MY_HASHTABLE_MAP_H_

#include "arraystorage.h"
#include "mypair.h"

#include <functional>

#include <cstddef>

namespace myhashtable{

	template<typename K, typename T, size_t MaxItems, size_t Size, template<typename,size_t,size_t> typename Storage = ArrayStorage, typename Hash = std::hash<K> >
	struct Map{
		using key_type		= K;
		using mapped_type	= T;
		using value_type	= MyPair<key_type, mapped_type>;

	public:
		constexpr Map() = default;

		template<typename... Ts>
		constexpr Map(Ts &&...ts) : data_(std::forward<Ts>(ts)...){}

	public:
		[[nodiscard]]
		constexpr static size_t size(){
			return Size;
		}

		[[nodiscard]]
		constexpr static size_t hash(key_type const &key){
			return Hash{}(key);
		}

		[[nodiscard]]
		constexpr static key_type    const &getKey(value_type const &data){
			return data.first;
		}

		[[nodiscard]]
		constexpr static mapped_type const &getVal(value_type const &data){
			return data.second;
		}

		[[nodiscard]]
		constexpr static mapped_type       &getVal(value_type       &data){
			return data.second;
		}

	public:
		[[nodiscard]]
		constexpr auto const &getStorage() const{
			return data_;
		}

		[[nodiscard]]
		constexpr auto       &getStorage(){
			return data_;
		}

	public:
		[[nodiscard]]
		constexpr bool equal(size_t id, key_type const &key) const{
			return getKey(data_[id]) == key;
		}

	private:
		Storage<value_type, MaxItems, Size> data_;
	};

} // namespace myhashtable

#endif


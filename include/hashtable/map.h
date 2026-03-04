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
		constexpr static size_t size(){
			return Size;
		}

		constexpr static size_t hash(key_type const &key){
			return Hash{}(key);
		}

		constexpr static key_type    const &getKey(value_type const &data){
			return data.first;
		}

		constexpr static mapped_type const &getVal(value_type const &data){
			return data.second;
		}

		constexpr static mapped_type       &getVal(value_type       &data){
			return data.second;
		}

	public:
		constexpr bool operator()(size_t id) const{
			return data_(id);
		}

		constexpr value_type const &operator[](size_t id) const{
			return data_[id];
		}

		constexpr value_type &operator[](size_t id){
			return data_[id];
		}

		template<typename... Ts>
		constexpr void emplace(size_t id, Ts &&...ts){
			data_.emplace(id, std::forward<Ts>(ts)...);
		}

	public:
		constexpr bool equal(size_t id, key_type const &key) const{
			return getKey(data_[id]) == key;
		}

		constexpr void stats() const{
			return data_.stats();
		}

	private:
		Storage<value_type, MaxItems, Size> data_;
	};

} // namespace myhashtable

#endif


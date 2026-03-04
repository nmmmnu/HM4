#ifndef MY_HASHTABLE_SET_H_
#define MY_HASHTABLE_SET_H_

#include "arraystorage.h"

#include <functional>

#include <cstddef>

namespace myhashtable{

	template<typename K, size_t MaxItems, size_t Size, template<typename,size_t,size_t> typename Storage = ArrayStorage, typename Hash = std::hash<K> >
	struct Set{
		using key_type		= K;
		using mapped_type	= key_type;
		using value_type	= key_type;

	public:
		constexpr Set() = default;

		template<typename... Ts>
		constexpr Set(Ts &&...ts) : data_(std::forward<Ts>(ts)...){}

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
		constexpr static key_type    const &getKey(key_type const &data){
			return data;
		}

		// there is no non-const version, because hashtable can be ruined
		[[nodiscard]]
		constexpr static mapped_type const &getVal(key_type const &data){
			return data;
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


#ifndef MY_HASHTABLE_STORAGE_ARRAY_H_
#define MY_HASHTABLE_STORAGE_ARRAY_H_

#include <array>

#include <cstddef>

namespace myhashtable{

	template<typename T, size_t MaxItems, size_t Size>
	struct ArrayStorage{
		static_assert(MaxItems < Size / 2, "MaxItems must be less than Size, optimally 20%");

	public:
		constexpr ArrayStorage() = default;

		template<typename UT>
		constexpr ArrayStorage(UT &&sentinel) : sentinel_(std::forward<UT>(sentinel)){
			for(auto &x : data_)
				x = sentinel_;
		}

	public:
		constexpr static size_t size(){
			return Size;
		}

		constexpr bool operator()(size_t id) const{
			return data_[id] == sentinel_;
		}

		constexpr T const &operator[](size_t id) const{
			return data_[id];
		}

		constexpr T &operator[](size_t id){
			return data_[id];
		}

		template<typename... Ts>
		constexpr void emplace(size_t id, Ts &&...ts){
			data_[id] = { std::forward<Ts>(ts)... };
		}

		constexpr static void stats(){
		}

	private:
		std::array<T, Size>	data_		{};
		T			sentinel_	{};
	};

} // namespace myhashtable

#endif


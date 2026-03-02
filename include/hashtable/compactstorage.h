#ifndef MY_HASHTABLE_STORAGE_COMPACT_H_
#define MY_HASHTABLE_STORAGE_COMPACT_H_

#include <array>
#include <vector>
#include <limits>

#include <cstdio>
#include <cstdint>

namespace myhashtable{

	namespace compact_storage_impl_{
		template <std::size_t N>
		using size_type =
			std::conditional_t<N <= std::numeric_limits<std::uint8_t >::max() - 1,	std::uint8_t	,
			std::conditional_t<N <= std::numeric_limits<std::uint16_t>::max() - 1,	std::uint16_t	,
			std::conditional_t<N <= std::numeric_limits<std::uint32_t>::max() - 1,	std::uint32_t	,
												std::uint64_t
			> > >;

		template <std::size_t N>
		constexpr size_type<N> sentinel = std::numeric_limits<size_type<N> >::max();
	}

	template<typename T, size_t Size, template<typename> typename Container = std::vector>
	struct CompactStorage{
		constexpr CompactStorage(size_t reserve = 0){
			if (reserve)
				data_.reserve(reserve);

			for(auto &x : link_)
				x = sentinel__;
		}

	public:
		constexpr static size_t size(){
			return Size;
		}

		constexpr bool operator()(size_t id) const{
			return link_[id] == sentinel__;
		}

		constexpr T const &operator[](size_t id) const{
			return data_[ link_[id] ];
		}

		constexpr T &operator[](size_t id){
			return data_[ link_[id] ];
		}

		template<typename... Ts>
		constexpr void emplace(size_t id, Ts &&...ts){
			if (operator()(id)){
				// the cell is empty, create new one...
				data_.emplace_back(std::forward<Ts>(ts)...);

				// populate link...
				link_[id] = static_cast<size_type>(data_.size() - 1);
			}else{
				// update in place...
				data_[ link_[id] ] = { std::forward<Ts>(ts)... };
			}
		}

		void stats() const{
			printf("Inserted items: %10zu\n", data_.size());
		}

	private:
		using size_type = compact_storage_impl_::size_type<Size>;

		constexpr static auto sentinel__ = compact_storage_impl_::sentinel<Size>;

	private:
		std::array<size_type,Size>	link_;
		Container<T>			data_;
	};

} // namespace myhashtable

#endif


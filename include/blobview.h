#ifndef BLOB_REF_H_
#define BLOB_REF_H_

#include <cstddef>	// size_t
#include <type_traits>	// is_pod

class BlobView{
public:
	constexpr BlobView() = default;

	constexpr BlobView(const void *mem, size_t const size) noexcept :
				mem_	(static_cast<const char *>(mem)	),
				size_	(size				){}

	template<typename T>
	constexpr BlobView(T const &x) noexcept :
				BlobView(x.data(), x.size()){}

	template<size_t N>
	constexpr BlobView(const char(&mem)[N]) noexcept:
				BlobView(mem, N){}

public:
	constexpr bool empty() const noexcept{
		return mem_ == nullptr || size_ == 0;
	}

	constexpr size_t size() const noexcept{
		return size_;
	}

	template <class T>
	constexpr size_t sizeAs() const noexcept{
		return size() / sizeof(T);
	}

	constexpr const void *data_() const noexcept{
		return mem_;
	}

	void reset() noexcept{
		mem_ = nullptr;
		size_ = 0;
	}

public:
	const void *safeAccessMemory(size_t const pos, size_t const size) const noexcept{
		if (empty() || size == 0 || pos + size > size_)
			return nullptr;

		return & mem_[pos];
	}

	const void *safeAccessMemory(const void *ptr, size_t const size) const noexcept{
		auto endpoint = [](const char *p, size_t size){
			return p + size;
		};

		const char *ptrc = static_cast<const char *>(ptr);

		if ( empty() || size == 0 || ptrc < mem_ || endpoint(ptrc, size) > endpoint(mem_, size_) )
			return nullptr;

		return ptr;
	}

public:
	template <class T>
	const T *asArray(size_t const pos, size_t const elements = 1) const noexcept{
		static_assert(std::is_pod<T>::value, "T must be POD type");

		return static_cast<const T *>( safeAccessMemory(pos * sizeof(T), elements * sizeof(T)) );
	}

	template <class T>
	const T *as(size_t const pos, size_t const elements = 1) const noexcept{
		static_assert(std::is_pod<T>::value, "T must be POD type");

		return static_cast<const T *>( safeAccessMemory(pos, elements * sizeof(T)) );
	}

	template <class T>
	const T *as(const void *ptr, size_t const elements = 1) const noexcept{
		static_assert(std::is_pod<T>::value, "T must be POD type");

		return static_cast<const T *>( safeAccessMemory(ptr, elements * sizeof(T)) );
	}

	// ambiguous call guard for 0
	template <class T>
	const T *as(int const pos, size_t const elements = 1) const noexcept{
		return pos < 0 ? nullptr : as<T>( static_cast<size_t>(pos), elements );
	}

private:
	const char	*mem_	= nullptr;
	size_t		size_	= 0;
};

#endif


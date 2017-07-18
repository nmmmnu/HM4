#ifndef BLOB_REF_H_
#define BLOB_REF_H_

#include <cstddef>	// size_t

#include <type_traits>	// is_pod

class BlobRef{
public:
	constexpr
	BlobRef() = default;

	constexpr
	BlobRef(const void *mem, size_t const size) noexcept :
				mem_( (const char *) mem ),
				size_(size){}

	template<size_t N>
	constexpr
	BlobRef(const char(&mem)[N]) noexcept:
				BlobRef(mem, N){}

public:
	constexpr
	bool empty() const noexcept{
		return mem_ == nullptr || size_ == 0;
	}

	constexpr
	size_t size() const noexcept{
		return size_;
	}

	constexpr
	const void *data_() const noexcept{
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
		const char *ptrc = (const char *) ptr;

		if (empty() || size == 0 || ptrc == nullptr || ptrc < mem_)
			return nullptr;

		/* long int */ auto const pos = ptrc - mem_;

		return safeAccessMemory( (size_t) pos, size);
	}

public:
	template <class T>
	const T *as(size_t const pos, size_t const elements = 1) const noexcept{
		static_assert(std::is_pod<T>::value, "T must be POD type");

		return reinterpret_cast<const T *>( safeAccessMemory(pos, elements * sizeof(T)) );
	}

	template <class T>
	const T *as(const void *ptr, size_t const elements = 1) const noexcept{
		static_assert(std::is_pod<T>::value, "T must be POD type");

		return reinterpret_cast<const T *>( safeAccessMemory(ptr, elements * sizeof(T)) );
	}

	// ambiguous call guard for 0
	template <class T>
	const T *as(int const pos = 0, size_t const elements = 1) const noexcept{
		return pos < 0 ?
			nullptr
		:
			as<T>( (size_t) pos, elements );
	}

private:
	const char	*mem_ = nullptr;
	size_t		size_ = 0;
};

#endif


#ifndef BLOB_GUARD_H_
#define BLOB_GUARD_H_

#include <cstddef>
#include <cstdint>
#include <type_traits>

class BlobGuard{
public:
	constexpr BlobGuard() = default;

	constexpr BlobGuard(const void *data, size_t const size) noexcept :
				data_(static_cast<const char *>(data)	),
				size_(size				){}

	template<typename Buffer,
			std::enable_if_t<
				// Buffer::value_type is byte
				std::is_same_v<typename Buffer::value_type, uint8_t> ||
				std::is_same_v<typename Buffer::value_type, int8_t > ||
				std::is_same_v<typename Buffer::value_type, char   > ||
				std::is_same_v<typename Buffer::value_type, void   >,
			int> = 0
	>
	constexpr BlobGuard(Buffer &buffer) :
				BlobGuard( buffer.data(), buffer.size() ){}

public:
	constexpr bool empty() const noexcept{
		return data_ == nullptr || size_ == 0;
	}

	constexpr size_t bytes() const noexcept{
		return size_;
	}

public:
	const void *safeAccessMemory(size_t const offset, size_t const size) const noexcept{
		if (empty() || size == 0 || offset + size > size_)
			return nullptr;

		return & data_[offset];
	}

	const void *safeAccessMemory(const void *ptr, size_t const size) const noexcept{
		auto endpoint = [](const char *p, size_t size){
			return p + size;
		};

		const char *ptrc = static_cast<const char *>(ptr);

		if ( empty() || size == 0 || ptrc < data_ || endpoint(ptrc, size) > endpoint(data_, size_) )
			return nullptr;

		return ptr;
	}

public:
	template <class T>
	constexpr size_t sizeAs() const noexcept{
		return size_ / sizeof(T);
	}

	template <class T>
	const T *as(size_t const offset = 0, size_t const elements = 1) const noexcept{
		static_assert(std::is_standard_layout_v<T>, "T must be POD type");

		return static_cast<const T *>( safeAccessMemory(offset, elements * sizeof(T)) );
	}

	template <class T>
	const T *as(const void *ptr, size_t const elements = 1) const noexcept{
		static_assert(std::is_standard_layout_v<T>, "T must be POD type");

		return static_cast<const T *>( safeAccessMemory(ptr, elements * sizeof(T)) );
	}

	// ambiguous call guard for 0
	template <class T>
	const T *as(int const offset, size_t const elements = 1) const noexcept{
		return offset < 0 ? nullptr : as<T>( static_cast<size_t>(offset), elements );
	}

	template <class T>
	auto asArray(size_t const offset = 0) const noexcept{
		static_assert(std::is_standard_layout_v<T>, "T must be POD type");

		struct Result{
			const T *data;
			size_t   size;
		};

		auto const elements = sizeAs<T>();

		return Result{
			static_cast<const T *>( safeAccessMemory(offset, elements * sizeof(T)) ),
			elements
		};
	}

private:
	const char	*data_	= nullptr;
	size_t		size_	= 0;
};

#endif


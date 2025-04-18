#ifndef MY_MMAP_BUFFER_
#define MY_MMAP_BUFFER_

#include <cstdint>
#include <new>		// std::bad_alloc, std::nothrow_t

#include "mybufferview.h"

namespace MyBuffer{

	namespace mmapbuffer_impl_{
		void *create(std::size_t size) noexcept;
		void destroy(void *p, std::size_t size) noexcept;

		void adviceNeed(void *p, std::size_t size) noexcept;
		void adviceFree(void *p, std::size_t size) noexcept;
	}

	struct MMapMemoryResource{
		using value_type = void;
		using size_type  = std::size_t;

		MMapMemoryResource() = default;

		explicit MMapMemoryResource(size_type size, std::nothrow_t) noexcept :
					size_(size				),
					data_(mmapbuffer_impl_::create(size_)	){}

		explicit MMapMemoryResource(size_type size) :
					size_(size			),
					data_(createOrThrow__(size_)	){}

		MMapMemoryResource(MMapMemoryResource &other) noexcept :
					size_(other.size_		),
					data_(other.data_		){
			other.size_ = 0;
			other.data_ = nullptr;
		}

		~MMapMemoryResource() noexcept {
			mmapbuffer_impl_::destroy(data_, size_);
		}

		operator bool() const noexcept{
			return data_;
		}

		void *data() noexcept{
			return data_;
		}

		const void *data() const noexcept{
			return data_;
		}

		std::size_t size() const noexcept{
			return size_;
		}

	private:
		static void *createOrThrow__(std::size_t size){
			if (size == 0)
				return nullptr;

			if (void *p = mmapbuffer_impl_::create(size); p)
				return p;

			throw std::bad_alloc{};
		}
	private:
		std::size_t	size_	= 0;
		void		*data_	= nullptr;
	};



	inline void mmapAdvice(ByteBufferView buffer, bool b){
		if (! buffer)
			return;

		if (b)
			mmapbuffer_impl_::adviceNeed(buffer.data(), buffer.size());
		else
			mmapbuffer_impl_::adviceFree(buffer.data(), buffer.size());
	}



	struct MMapAdviceGuard{
		MMapAdviceGuard(ByteBufferView buffer) : buffer(buffer){
			mmapAdvice(buffer, true);
		}

		~MMapAdviceGuard(){
			mmapAdvice(buffer, false);
		}

	private:
		ByteBufferView buffer;
	};

} // namespace MyBuffer

#endif


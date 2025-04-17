#ifndef MY_MMAP_BUFFER_
#define MY_MMAP_BUFFER_

#include <cassert>
#include <cstdint>

#include "mybufferview.h"

namespace MyBuffer{

	namespace mmapbuffer_impl_{
		void *createNormal(std::size_t size) noexcept;
		#ifdef USE_HUGETLB
		void *createHugeTLB(std::size_t size) noexcept;
		#endif
		void destroy(void *p, std::size_t size) noexcept;

		void adviceNeed(void *p, std::size_t size) noexcept;
		void adviceFree(void *p, std::size_t size) noexcept;

		inline void *allocate__(std::size_t size){
			if (size == 0)
				return nullptr;

			return
				#ifdef USE_HUGETLB
					createHugeTLB(size)
				#else
					createNormal(size)
				#endif
			;
		}
	};

	struct MMapMemoryResource{
		using value_type = void;
		using size_type  = std::size_t;

		MMapMemoryResource() = default;

		MMapMemoryResource(size_type size) : size_(size){}

		MMapMemoryResource(MMapMemoryResource &other) : size_(other.size_), data_(other.data_){
			other.size_ = 0;
			other.data_ = nullptr;
		}

		~MMapMemoryResource(){
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
		std::size_t	size_	= 0;
		void		*data_	= mmapbuffer_impl_::allocate__(size_);
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


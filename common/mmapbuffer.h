#ifndef MY_MMAP_BUFFER_
#define MY_MMAP_BUFFER_

#include <cassert>
#include <cstdint>

namespace MyBuffer{

	namespace mmapbuffer_impl_{
		void *createNormal(std::size_t size) noexcept;
		#ifdef USE_HUGETLB
		void *createHugeTLB(std::size_t size) noexcept;
		#endif
		void destroy(void *p, std::size_t size) noexcept;

		void adviceNeed(void *p, std::size_t size) noexcept;
		void adviceFree(void *p, std::size_t size) noexcept;
	};

	struct MMapBufferResource{
		using value_type = char;
		using size_type  = std::size_t;

		MMapBufferResource(size_type size) : size_(size){}

		MMapBufferResource(MMapBufferResource &other) : size_(other.size_), data_(other.data_){
			other.size_ = 0;
			other.data_ = nullptr;
		}

		~MMapBufferResource(){
			mmapbuffer_impl_::destroy(data_, size_);
		}

		operator bool() const noexcept{
			return data_;
		}

		value_type *data() noexcept{
			return data_;
		}

		const value_type *data() const noexcept{
			return data_;
		}

		size_type size() const noexcept{
			return size_;
		}

	private:
		static value_type *allocate__(std::size_t size){
			return static_cast<value_type *>(
				#ifdef USE_HUGETLB
					mmapbuffer_impl_::createHugeTLB(size)
				#else
					mmapbuffer_impl_::createNormal(size)
				#endif
			);
		}

	private:
		size_type	size_;
		value_type	*data_ = allocate__(size_);
	};



	template<typename Buffer>
	void adviceNeeded(Buffer &buffer, bool b){
		if (! buffer)
			return;

		if (b)
			mmapbuffer_impl_::adviceNeed(buffer.data(), buffer.size());
		else
			mmapbuffer_impl_::adviceFree(buffer.data(), buffer.size());
	}



	template<typename Buffer>
	struct AdviceNeededGuard{
		constexpr AdviceNeededGuard(Buffer &buffer) : buffer_(buffer){
			adviceNeeded(buffer_, true);
		}

		~AdviceNeededGuard(){
			adviceNeeded(buffer_, false);
		}

	private:
		Buffer &buffer_;
	};

} // namespace MyBuffer

#endif


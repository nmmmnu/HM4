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

	template<typename T>
	struct MMapBuffer{
		using value_type	= T;
		using size_type		= std::size_t;

		MMapBuffer(size_type size = 1) : size_(size){}

		MMapBuffer(MMapBuffer &other) : size_(other.size_), data_(other.data_){
			other.size_ = 0;
			other.data_ = nullptr;
		}

		~MMapBuffer(){
			mmapbuffer_impl_::destroy(data_, bytes());
		}

		value_type *data() noexcept{
			return data_;
		}

		const value_type *data() const noexcept{
			return data_;
		}

		value_type &operator*() noexcept{
			return *data_;
		}

		value_type const &operator*() const noexcept{
			return *data_;
		}

		const value_type *operator->() const noexcept{
			return data_;
		}

		value_type *operator->() noexcept{
			return data_;
		}

		constexpr const value_type &operator[](std::size_t const index) const noexcept{
			return data_[index];
		}

		constexpr
		value_type &operator[](std::size_t const index) noexcept{
			return data_[index];
		}

		auto size() const noexcept{
			return size_;
		}

		auto bytes() const noexcept{
			return sizeof(T) * size_;
		}

	private:
		static value_type *allocate__(size_type size){
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
		value_type	*data_ = allocate__(sizeof(T) * size_);
	};

	using ByteMMapBuffer = MMapBuffer<std::uint8_t>;

	template<typename T>
	void adviceNeeded(MMapBuffer<T> &buffer, bool b){
		if (b)
			mmapbuffer_impl_::adviceNeed(buffer.data(), buffer.size());
		else
			mmapbuffer_impl_::adviceFree(buffer.data(), buffer.size());
	}

} // namespace MyBuffer

#endif


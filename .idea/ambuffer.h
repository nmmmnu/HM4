#ifndef AM_BUFFER_H_
#define AM_BUFFER_H_

#include <cstring>
#include <cstdio>

#include <utility>	// std::move

namespace am_buffer{

	template<typename AllocatorManager>
	class AMBuffer{
		using AllocatorContext	= typename AllocatorManager::Context;

		AllocatorManager	*man_;		// 8 bytes
		AllocatorContext	context_;	// 8 + 4 + 1 = 16 bytes

	public:
		using size_type		= typename AllocatorContext::size_type;

	public:
		constexpr AMBuffer(AllocatorManager &man) noexcept : man_(&man){}

		constexpr AMBuffer(AllocatorManager &man, size_t size) : AMBuffer(man){
			[[maybe_unused]]
			bool b = reserve(size);
		}

		constexpr AMBuffer(AMBuffer &&other) noexcept :
						man_		(std::move(other.man_		)),
						context_	(std::move(other.context_	)){

			other.clear_();
		}

		constexpr AMBuffer &operator=(AMBuffer &&other) noexcept{
			if (this != &other)
				swap(other);

			return *this;
		}

		~AMBuffer() noexcept{
			deallocate_();
		}

		void swap(AMBuffer &other) noexcept{
			using std::swap;

			swap(man_	, other.man_		);
			swap(context_	, other.context_	);
		}

	public:
		[[nodiscard]]
		bool reserve(size_t size){
			return allocate_(size);
		}

		constexpr void clear() noexcept{
			deallocate_();
		}

	public:
		[[nodiscard]]
		bool resize(size_t sizeNew){
			if (sizeNew > capacity())
				if (!reserve(sizeNew))
					return false;

			context_.size = static_cast<size_type>(sizeNew);

			return true;
		}

	//	[[nodiscard]]
	//	bool push(char c){
	//		if (!reserveCheck_(1))
	//			return false;
	//
	//		context_.data[context_.size++] = c;
	//
	//		return true;
	//	}
	//
		[[nodiscard]]
		char *push_(size_t size){
			if (!reserveCheck_(size))
				return nullptr;

			auto *dst = context_.data + context_.size;

			context_.size += size;

			return dst;
		}

		[[nodiscard]]
		bool push_(const char *src, size_t size){
			char *dst = push_(size);

			if (!dst)
				return false;

			memcpy(dst, src, size);

			return true;
		}

	public:
		constexpr const char *data() const noexcept{
			return context_.data;
		}

		constexpr char *data() noexcept{
			return context_.data;
		}

		constexpr size_type size() const noexcept{
			return context_.size;
		}

		constexpr size_type capacity() const noexcept{
			return context_.capacity;
		}

		void print() const noexcept{
			printf("ptr = %-4s : size = %9u : capacity = %9u\n",
				data() ? "OK" : "NULL"	,
				size()			,
				capacity()
			);
		}

	private:
		[[nodiscard]]
		bool reserveCheck_(size_t size){
			if (context_.size + size > capacity())
				return reserve(context_.size + size);
			else
				return true;
		}

		constexpr void clear_() noexcept{
			context_.clear();
		}

		bool allocate_(size_t size){
			printf("%p Allocate: %zu\n", (void *) this, size);
			printf("- old state: (%p) %zu\n", (void *)context_.data, (size_t) context_.capacity);

			bool const b = man_->allocate(context_, size);

			printf("- new state: (%p) %zu\n", (void *)context_.data, (size_t) context_.capacity);

			return b;
		}

		void deallocate_() noexcept{
			if (context_.data){
				printf("%p Deallocate:\n", (void *) this);
				printf("- old state: (%p) %zu\n", (void *)context_.data, (size_t) context_.capacity);
			}

			// still go there, because it may clear the context
			man_->deallocate(context_);
		}
	};



	template<typename AllocatorManager>
	void swap(AMBuffer<AllocatorManager> &a, AMBuffer<AllocatorManager> &b) noexcept{
		a.swap(b);
	}

} // namespace am_buffer

#endif


#ifndef _IO_BUFFER_H
#define _IO_BUFFER_H

#include <string_view>

#include <cstdio>
#include <cassert>
#include <cstring>	// memcpy

namespace net{

template<typename T>
struct IOBuffer{
	using MemoryManager	= T;
	using MemoryNodePtr	= typename MemoryManager::NodePtr;
	using size_type		= typename MemoryManager::size_type;

private:
	constexpr static size_t STANDARD_READ_SIZE = 4096;

private:
	MemoryNodePtr node = nullptr; // this is a pointer

public:
	constexpr IOBuffer() noexcept = default;

	IOBuffer(MemoryNodePtr &&node) noexcept : node(std::move(node)){
		// just in case
		node = nullptr;
	}

	IOBuffer(IOBuffer &&other) noexcept : node(std::move(other.node)){
		other.node = nullptr;
	}

	IOBuffer &operator=(IOBuffer &&other) noexcept{
		assert(node == nullptr && "You can assign only on deallocated IOBuffer!");

		node = std::move(other.node);

		other.node = nullptr;

		return *this;
	}

	~IOBuffer() noexcept{
		assert(node == nullptr && "You forgot to deallocate the memory!");
	}

public:
	void swap(MemoryNodePtr &node) noexcept{
		using std::swap;

		swap(this->node, node);
	}

	void swap(IOBuffer &other) noexcept{
		using std::swap;

		swap(node, other.node);
	}

public:
	[[nodiscard]]
	const char *data() const noexcept{
		return node ? node->data + node->head : nullptr;
	}

	[[nodiscard]]
	size_t size() const noexcept{
		return node ? node->size - node->head: 0;
	}

	[[nodiscard]]
	size_t sizeFull() const noexcept{
		return node ? node->size : 0;
	}

	[[nodiscard]]
	size_t capacity() const noexcept{
		return node ? node->capacity : 0;
	}

	[[nodiscard]]
	explicit operator std::string_view() const noexcept{
		return std::string_view{ data(), size() };
	}

public:
	[[nodiscard]]
	bool reserve(MemoryManager &mm, size_t const size){
		return mm.reallocate(node, size);
	}

	void deallocate(MemoryManager &mm){
		return mm.deallocate(node);
	}

	void clear(MemoryManager &mm){
		deallocate(mm);
	}

public:
	template<typename DataLoader>
	[[nodiscard]]
	bool bulkLoad(MemoryManager &mm, DataLoader const &dl) noexcept{
		return mm.bulkLoad(node, dl);
	}

	[[nodiscard]]
	auto provideBufferAtLeast(MemoryManager &mm, size_t const size = STANDARD_READ_SIZE) noexcept{
		assert(size && "Size must be great than zero");

		struct Result{
			char	*data	= nullptr;
			size_t	size	= 0;
		};

		auto const sizeTotal = sizeFull();

		if (!reserve(mm, sizeTotal + size))
			return Result{};

		return Result{
			getIndex_(sizeTotal),
			capacity() - sizeTotal
		};
	}

	[[nodiscard]]
	char *provideBuffer(MemoryManager &mm, size_t const size = STANDARD_READ_SIZE) noexcept{
		auto r = provideBufferAtLeast(mm, size);

		return r.data;
	}

	void finalizeBuffer(MemoryManager &, const char *offset, size_t const actualSize){
		assert(node && "Must be run on allocated memory");

		auto const size = static_cast<size_t>(offset - node->data) + actualSize;

		node->size = static_cast<size_type>(size);
	}

public:
	[[nodiscard]]
	bool push(MemoryManager &mm, const char *ptr, size_t const size) noexcept{
		if (!size)
			return false;

		char *p = provideBuffer(mm, size);

		if (!p)
			return false;

		memcpy(p, ptr, size);

		finalizeBuffer(mm, p, size);

		return true;
	}

	[[nodiscard]]
	bool push(MemoryManager &mm, std::string_view const s) noexcept{
		return push(mm, s.data(), s.size());
	}

public:
	bool pop(MemoryManager &mm, size_t const size) noexcept{
		if (!size)
			return false;

		auto const availableSize = this->size();

		if (availableSize < size)
			return false;

		if (availableSize == size){
			deallocate(mm);

			return true;
		}

		node->head += size;

		return true;
	}

public:
	void print(bool const body = true) const{
		if (!node)
			return;

		printf("h: %10zu | s: %10zu | c: %10zu | %.*s\n",
			(size_t) node->head	,
			size()			,
			capacity()		,
			(int) size()		,
			body ? data() : "[data]"
		);
	}

private:
	[[nodiscard]]
	char *getIndex_(size_t index) noexcept{
		assert(node && "Must be run on allocated memory");
		return node->data + index;
	}
};



template<typename MemoryManager>
void swap(IOBuffer<MemoryManager> &a, IOBuffer<MemoryManager> &b){
	a.swap(b);
}

} // namespace

#endif


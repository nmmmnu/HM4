#ifndef ALLOCATED_STRING_H_
#define ALLOCATED_STRING_H_

#include "mystring.h"
#include "baseallocator.h"



template<typename Allocator, typename... Args>
std::string_view concatenateAllocatedBuffer(Allocator &allocator, Args &&... args){
	static_assert((std::is_constructible_v<std::string_view, Args> && ...));

	size_t const reserve_size = (std::string_view{ args }.size() + ...);

	char *buffer = MyAllocator::allocate<char>(allocator, reserve_size);

	if (!buffer)
		return "";

	return concatenateRawBuffer_(buffer, std::forward<Args>(args)...);
}



#endif


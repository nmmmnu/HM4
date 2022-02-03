#ifndef DISK_FILE_PREDICATE_H_
#define DISK_FILE_PREDICATE_H_

#include <limits>

namespace hm4{
namespace flusher{

class DiskFilePredicate{
	constexpr static size_t minBytes = Pair::maxBytes() + 1024u;

public:
	DiskFilePredicate() = default;
	DiskFilePredicate(size_t const maxSize) : maxSize_(maxSize){}

	template<class List>
	bool operator()(List const &list) const{
		return	list.bytes() > maxSize_				||
			list.getAllocator().getFreeMemory() < minBytes
		;
	}

private:
	size_t maxSize_ = std::numeric_limits<size_t>::max();
};

} // namespace flusher
} // namespace

#endif


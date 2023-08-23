#ifndef DISK_FILE_PREDICATE_H_
#define DISK_FILE_PREDICATE_H_

namespace hm4::flusher{



struct DiskFileAllocatorPredicate{
	template<class List>
	[[deprecated]]
	bool operator()(List const &list) const{
		return list.getAllocator().getFreeMemory() < minBytes;
	}

	template<class List>
	bool operator()(List const &list, size_t const bytes) const{
		return list.getAllocator().getFreeMemory() < bytes + 1024u;
	}

private:
	constexpr static size_t minBytes = Pair::maxBytes() + 1024u;
};



} // hm4::flusher

#endif


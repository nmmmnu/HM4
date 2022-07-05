#ifndef DISK_FILE_PREDICATE_H_
#define DISK_FILE_PREDICATE_H_

namespace hm4{
namespace flusher{



struct DiskFileAllocatorPredicate{
	template<class List>
	bool operator()(List const &list) const{
		return list.getAllocator().getFreeMemory() < minBytes;
	}

private:
	constexpr static size_t minBytes = Pair::maxBytes() + 1024u;
};


#if 0
struct DiskFilePredicate : DiskFileAllocatorPredicate{
	DiskFilePredicate(size_t const maxSize) : maxSize_(maxSize){}

	template<class List>
	bool operator()(List const &list) const{
		return	DiskFileAllocatorPredicate::operator()(list)	||
			list.bytes() > maxSize_
		;
	}

private:
	size_t maxSize_;
};
#endif


} // namespace flusher
} // namespace

#endif


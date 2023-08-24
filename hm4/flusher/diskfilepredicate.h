#ifndef DISK_FILE_PREDICATE_H_
#define DISK_FILE_PREDICATE_H_

namespace hm4::flusher{



struct DiskFileAllocatorPredicate{
	template<class List>
	bool operator()(List const &list, size_t const bytes) const{
		auto const minBytes =
				bytes +
				list.mutable_list().INTERNAL_NODE_SIZE +
				8u
		;

		return list.getAllocator().getFreeMemory() <  minBytes;
	}
};



} // hm4::flusher

#endif


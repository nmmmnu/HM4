#ifndef ITERATOR_LIST_LOADER_H_
#define ITERATOR_LIST_LOADER_H_

#include "baselistloader.h"

namespace hm4{
namespace listloader{


template<class IT>
class IteratorListLoader{
public:
	using DiskList	= hm4::disk::DiskList;
	using List 	= const impl_::ContainerHelper::CollectionList;

public:
	IteratorListLoader(IT first, IT last, DiskList::VMAllocator *allocator, DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				container_(allocator, mode){

		container_.copy(first, last);
	}

	IteratorListLoader(IT first, IT last, DiskList::VMAllocator &allocator, DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				IteratorListLoader(first, last, &allocator, mode){}

	IteratorListLoader(IT first, IT last, DiskList::NoVMAllocator,          DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				IteratorListLoader(first, last, nullptr   , mode){}

	// reload not supported
	// Command pattern not supported

	List const &getList() const{
		return container_.getList();
	}

private:
	impl_::ContainerHelper	container_;
};


} // namespace listloader
} // namespace

#endif


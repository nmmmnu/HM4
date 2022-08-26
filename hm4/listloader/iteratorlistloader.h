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
	IteratorListLoader(IT first, IT last, MMAPFile::Advice const advice = DiskList::DEFAULT_ADVICE, DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				container_(advice, mode){

		container_.copy(first, last);
	}

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


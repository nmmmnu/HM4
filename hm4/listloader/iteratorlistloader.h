#ifndef ITERATOR_LIST_LOADER_H_
#define ITERATOR_LIST_LOADER_H_

#include "baselistloader.h"

#include "mynarrow.h"



namespace hm4{
namespace listloader{


class IteratorListLoader{
public:
	using DiskList	= hm4::disk::DiskList;
	using List 	= const impl_::ContainerHelper::CollectionList;

public:
	IteratorListLoader(const char **first, const char **last, MMAPFile::Advice const advice = DiskList::DEFAULT_ADVICE, DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				container_(advice, mode),
				first_(first),
				last_(last){
		refresh();
	}

	bool refresh(){
		container_.clear();
		container_.reserve(narrow<size_t>(std::distance(first_, last_)));

		for(auto it = std::make_reverse_iterator(last_); it != std::make_reverse_iterator(first_); ++it)
			container_.push_back(*it);

		return true;
	}

	// Command pattern
	int command(int = 0){
		return refresh();
	}

	/* const */ List &getList() const{
		return container_.getList();
	}

private:
	impl_::ContainerHelper	container_;

	const char **first_;
	const char **last_;
};


} // namespace listloader
} // namespace

#endif


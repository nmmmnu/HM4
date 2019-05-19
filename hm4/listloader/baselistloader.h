#ifndef BASE_TABLE_LOADER_H_
#define BASE_TABLE_LOADER_H_

#include "disk/disklist.h"

#include <vector>

namespace hm4{
namespace listloader{
namespace baselistloader_impl_{

class BaseListLoader{
public:
	using DiskList		= disk::DiskList;
	using container_type	= std::vector<DiskList>;
	using iterator		= container_type::const_iterator;

public:
	static constexpr MMAPFile::Advice	DEFAULT_ADVICE	= DiskList::DEFAULT_ADVICE;
	static constexpr DiskList::OpenMode	DEFAULT_MODE	= DiskList::OpenMode::MINIMAL;


protected:
	BaseListLoader(MMAPFile::Advice const advice, DiskList::OpenMode const mode) :
				advice_(advice),
				mode_(mode){}

public:
	auto begin() const{
		return std::begin(container_);
	}

	auto end() const{
		return std::end(container_);
	}

protected:
	void insert_(const StringRef &filename){
		container_.emplace_back();
                container_.back().open(filename, advice_, mode_);
	}

protected:
	container_type		container_;

private:
	MMAPFile::Advice	advice_;
	DiskList::OpenMode	mode_;
};


} // namespace baselistloader_impl_
} // namespace listloader
} // namespace


#endif


#ifndef BASE_TABLE_LOADER_H_
#define BASE_TABLE_LOADER_H_

#include "disk/disktable.h"

#include <vector>

namespace hm4{
namespace tableloader{
namespace tableloader_impl_{


class BaseTableLoader{
public:
	using DiskTable		= disk::DiskTable;
	using container_type	= std::vector<DiskTable>;

protected:
	BaseTableLoader(int const advice = DiskTable::DEFAULT_ADVICE) :
				advice_(advice){}

public:
	const container_type &operator*() const{
		return container_;
	}


protected:
	void insert_(const StringRef &filename){
		container_.emplace_back();
                container_.back().open(filename, advice_);
	}

protected:
	container_type	container_;

private:
	int		advice_;
};


} // namespace tableloader_impl_
} // namespace tableloader
} // namespace


#endif


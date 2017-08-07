#ifndef BASE_TABLE_LOADER_H_
#define BASE_TABLE_LOADER_H_

#include "disk/disklist.h"

#include <vector>

namespace hm4{
namespace listloader{
namespace listloader_impl_{


class BaseListLoader{
public:
	using DiskList		= disk::DiskList;
	using container_type	= std::vector<DiskList>;

protected:
	static int getDefautAdvice__(){
		return DiskList::DEFAULT_ADVICE;
	}

	BaseListLoader(int const advice = getDefautAdvice__()) :
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


} // namespace listloader_impl_
} // namespace listloader
} // namespace


#endif


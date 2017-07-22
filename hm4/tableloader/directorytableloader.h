#ifndef _DIRECTORY_LSM_CONTAINER_H
#define _DIRECTORY_LSM_CONTAINER_H

#include <string>
#include <vector>

#include "disk/disktable.h"


namespace hm4{
namespace tableloader{


class DirectoryTableLoader{
public:
	using container_type	= std::vector<disk::DiskTable>;

public:
	DirectoryTableLoader(const StringRef &path) : path_(path){
		refresh();
	}

	const container_type &operator*() const{
		return container_;
	}

	bool refresh();

private:
	std::string	path_;
	container_type	container_;
};


} // namespace tableloader
} // namespace


#endif


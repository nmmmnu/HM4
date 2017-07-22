#ifndef _FILE_LSM_CONTAINER_ARGV_H
#define _FILE_LSM_CONTAINER_ARGV_H

#include <vector>

#include "disk/disktable.h"


namespace hm4{
namespace tableloader{


class ArgTableLoader{
public:
	using container_type	= std::vector<disk::DiskTable>;

public:
	ArgTableLoader(int const argc, const char **argv) :
				argc_(argc),
				argv_(argv){
		refresh_();
	}

	const container_type &operator*() const{
		return container_;
	}

private:
	bool refresh_();

private:
	int		argc_;
	const char	**argv_;

	container_type	container_;
};


} // namespace tableloader
} // namespace


#endif


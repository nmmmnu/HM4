#ifndef DIRECTORY_TABLE_LOADER_H_
#define DIRECTORY_TABLE_LOADER_H_

#include "basetableloader.h"


namespace hm4{
namespace tableloader{


class DirectoryTableLoader : public tableloader_impl_::BaseTableLoader{
public:
	using DiskTable		= disk::DiskTable;
	using container_type	= std::vector<DiskTable>;

public:
	DirectoryTableLoader(const StringRef &path, int const advice = DiskTable::DEFAULT_ADVICE) :
				BaseTableLoader(advice),
				path_(path){
		refresh();
	}

	bool refresh();

private:
	std::string	path_;
};


} // namespace tableloader
} // namespace


#endif


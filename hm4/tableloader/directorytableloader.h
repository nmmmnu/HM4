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
	DirectoryTableLoader() = default;

	DirectoryTableLoader(std::string path, int const advice = DiskTable::DEFAULT_ADVICE) :
				BaseTableLoader(advice),
				path_(std::move(path)){
		refresh_();
	}

	bool refresh(){
		refresh_();

		return true;
	}

private:
	void refresh_();

private:
	std::string	path_;
};


} // namespace tableloader
} // namespace


#endif


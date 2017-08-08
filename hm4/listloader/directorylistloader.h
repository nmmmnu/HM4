#ifndef DIRECTORY_TABLE_LOADER_H_
#define DIRECTORY_TABLE_LOADER_H_

#include "baselistloader.h"


namespace hm4{
namespace listloader{


class DirectoryListLoader : public listloader_impl_::BaseListLoader{
public:
	using container_type	= std::vector<DiskList>;

public:
//	DirectoryListLoader() = default;

	DirectoryListLoader(std::string path, int const advice = getDefautAdvice__()) :
				BaseListLoader(advice),
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


} // namespace listloader
} // namespace


#endif


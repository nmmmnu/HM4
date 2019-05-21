#ifndef ARG_TABLE_LOADER_H_
#define ARG_TABLE_LOADER_H_

#include "baselistloader.h"

#include "mynarrow.h"

namespace hm4{
namespace listloader{


template<class Container>
class ArgListLoader{
public:
	using DiskList = hm4::disk::DiskList;

	ArgListLoader(Container &container, int const argc, const char **argv, MMAPFile::Advice const advice = DEFAULT_ADVICE, DiskList::OpenMode const mode = DEFAULT_MODE) :
				inserter_(container, advice, mode),
				argc_(argc),
				argv_(argv){
		refresh();
	}

	bool refresh(){
		inserter_.clear();
		inserter_.reserve(narrow<size_t>(argc_));

		for(int i = argc_; i --> 0;)
			inserter_(argv_[i]);

		return true;
	}

	// Command pattern
	int command(int = 0){
		return refresh();
	}

private:
	impl_::Inserter<Container>	inserter_;

	int				argc_	= 0;
	const char			**argv_;
};


} // namespace listloader
} // namespace


#endif


#ifndef ARG_TABLE_LOADER_H_
#define ARG_TABLE_LOADER_H_

#include "basetableloader.h"

namespace hm4{
namespace tableloader{


class ArgTableLoader : public tableloader_impl_::BaseTableLoader{
public:
	ArgTableLoader(int const argc, const char **argv, int const advice = DiskTable::DEFAULT_ADVICE) :
				BaseTableLoader(advice),
				argc_(argc),
				argv_(argv){
		refresh();
	}

	bool refresh();

private:
	int		argc_;
	const char	**argv_;
};


} // namespace tableloader
} // namespace


#endif


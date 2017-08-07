#ifndef ARG_TABLE_LOADER_H_
#define ARG_TABLE_LOADER_H_

#include "baselistloader.h"

namespace hm4{
namespace listloader{


class ArgListLoader : public listloader_impl_::BaseListLoader{
public:
	ArgListLoader() = default;

	ArgListLoader(int const argc, const char **argv, int const advice = getDefautAdvice__()) :
				BaseListLoader(advice),
				argc_(argc),
				argv_(argv){
		refresh();
	}

	bool refresh();

private:
	int		argc_	= 0;
	const char	**argv_;
};


} // namespace listloader
} // namespace


#endif


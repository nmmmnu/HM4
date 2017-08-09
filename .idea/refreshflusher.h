#ifndef REFRESH_FLUSH_H_
#define REFRESH_FLUSH_H_

namespace hm4{
namespace flusher{


class RefreshFlusher{
public:
	template<class LIST>
	bool operator << (const LIST &) const{
		return true;
	}
};


} // namespace flusher
} // namespace

#endif



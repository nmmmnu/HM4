#ifndef MUTABLE_DBADAPTER_H_
#define MUTABLE_DBADAPTER_H_

#include "imutabledbadapter.h"

template<class LIST, class LOADER>
class MutableDBAdapter : public ImutableDBAdapter<LIST, LOADER>{
public:
	using Pair = hm4::Pair;
	constexpr static size_t DEFAULT_MAX_RESULTS = ImutableDBAdapter<LIST, LOADER>::DEFAULT_MAX_RESULTS;

public:
	constexpr static bool IS_MUTABLE = true;

public:
	MutableDBAdapter(LIST &list, LOADER &loader, size_t const maxResults = DEFAULT_MAX_RESULTS) :
				ImutableDBAdapter<LIST, LOADER>(list, loader, maxResults),
				list_(list){}

	MutableDBAdapter(LIST &list, size_t const maxResults = DEFAULT_MAX_RESULTS) :
				ImutableDBAdapter<LIST, LOADER>(list, maxResults),
				list_(list){}

public:
	void set(const StringRef &key, const StringRef &val, const StringRef & = {} ){
		assert(!key.empty());

		list_.emplace( key, val );
	}

	bool del(const StringRef &key){
		assert(!key.empty());

		return list_.erase(key);
	}

private:
	LIST	&list_;
};


#endif


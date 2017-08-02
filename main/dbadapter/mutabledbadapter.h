#ifndef MUTABLE_DBADAPTER_H_
#define MUTABLE_DBADAPTER_H_

#include "dbadapter.h"

#include "blackholelist.h"


template<class LIST, class LOADER>
class MutableDBAdapter : public DBAdapter<LIST, LOADER>{
public:
	using Pair = hm4::Pair;
	constexpr static size_t DEFAULT_MAX_RESULTS = DBAdapter<LIST, LOADER>::DEFAULT_MAX_RESULTS;

public:
	constexpr static std::true_type IS_MUTABLE{};

public:
	MutableDBAdapter(LIST &list, /* optional */ LOADER *loader, size_t const maxResults = DEFAULT_MAX_RESULTS) :
				DBAdapter<LIST, LOADER>(list, loader, maxResults),
				list_(list){}

public:
	void set(const StringRef &key, const StringRef &val, const StringRef & = {} ){
		assert(! key.empty());

		insert_( { key, val } );
	}

	bool del(const StringRef &key){
		assert(! key.empty());

		insert_( Pair::tombstone(key) );
		return true;
	}

private:
	void insert_(Pair &&p){
		hm4::BlackHoleList xlist_;

		if (p)
			xlist_.insert(std::move(p));
	}

private:
	LIST	&list_;
};


#endif


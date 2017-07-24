#ifndef _FLUSH_LIST_H
#define _FLUSH_LIST_H

#include "ilist.h"


namespace hm4{


template <class LIST, class FLUSH, class TABLELOADER = std::nullptr_t>
class FlushList : public IMutableList<FlushList<LIST, FLUSH> >{
public:
	constexpr static size_t MAX_SIZE = 1 * 1024 * 1024;

	using Iterator	= typename LIST::Iterator;
	using size_type	= typename LIST::size_type;

private:
	template <class UFLUSH>
	FlushList(LIST &list, UFLUSH &&flusher, TABLELOADER *loader, size_t const maxSize = MAX_SIZE) :
					list_(list),
					flusher_(std::forward<UFLUSH>(flusher)),
					loader_(loader),
					maxSize_(maxSize > MAX_SIZE ? maxSize : MAX_SIZE){}

public:
	template <class UFLUSH>
	FlushList(LIST &list, UFLUSH &&flusher, TABLELOADER &loader, size_t const maxSize = MAX_SIZE) :
					FlushList(list, std::forward<UFLUSH>(flusher), &loader, maxSize){}

	template <class UFLUSH>
	FlushList(LIST &list, UFLUSH &&flusher, size_t const maxSize = MAX_SIZE) :
					FlushList(list, std::forward<UFLUSH>(flusher), nullptr, maxSize){}

	FlushList(FlushList &&other) = default;

	~FlushList(){
		flush();
	}

	LIST &getList(){
		return list_;
	}

public:
	bool clear(){
		return list_.removeAll();
	}

	ObserverPair operator[](const StringRef &key) const{
		return Pair::observer(list_[key]);
	}

	bool remove(const StringRef &key){
		return list_.remove(key);
	}

	size_t bytes() const{
		return list_.bytes();
	}

	size_type size() const{
		return list_.size();
	}

private:
	friend class IMutableList<FlushList<LIST, FLUSH> >;

	template <class UPAIR>
	bool insertT_(UPAIR &&data){
		bool const result = list_.insert( std::forward<UPAIR>(data) );

		if (list_.bytes() > maxSize_){
			flush();
			list_.clear();
			notifyLoader_(loader_);
		}

		return result;
	}

public:
	Iterator begin() const{
		return list_.begin();
	}

	Iterator end() const{
		return list_.end();
	}

public:
	bool flush(){
		return flusher_ << list_;
	}

private:
	template<class T>
	static bool notifyLoader_(T *loader){
		if (loader)
			return loader->refresh();

		return false;
	}

	static bool notifyLoader_(std::nullptr_t *){
		return false;
	}

private:
	LIST		&list_;
	FLUSH		flusher_;
	TABLELOADER	*loader_;
	size_t		maxSize_;
};


} // namespace


#endif

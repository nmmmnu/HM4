#ifndef _FLUSH_LIST_H
#define _FLUSH_LIST_H

#include "ilist.h"


namespace hm4{


template <class LIST, class FLUSH, class LIST_LOADER = std::nullptr_t>
class FlushList : public IList<FlushList<LIST, FLUSH>, true>{
public:
	constexpr static size_t MAX_SIZE = 1 * 1024 * 1024;

	using Iterator		= typename LIST::Iterator;
	using size_type		= typename LIST::size_type;

private:
	template <class UFLUSH>
	FlushList(LIST &list, UFLUSH &&flusher, LIST_LOADER *loader, size_t const maxSize = MAX_SIZE) :
					list_(& list),
					flusher_(std::forward<UFLUSH>(flusher)),
					loader_(loader),
					maxSize_(maxSize > MAX_SIZE ? maxSize : MAX_SIZE){}

public:
	template <class UFLUSH>
	FlushList(LIST &list, UFLUSH &&flusher, LIST_LOADER &loader, size_t const maxSize = MAX_SIZE) :
					FlushList(list, std::forward<UFLUSH>(flusher), &loader, maxSize){}

	template <class UFLUSH>
	FlushList(LIST &list, UFLUSH &&flusher, size_t const maxSize = MAX_SIZE) :
					FlushList(list, std::forward<UFLUSH>(flusher), nullptr, maxSize){}

	~FlushList(){
		flush();
	}

	LIST &getList(){
		assert(list_);

		return *list_;
	}

private:
	friend class IList<FlushList<LIST, FLUSH>, true>;

	template <class UPAIR>
	bool insertT_(UPAIR &&data){
		assert(list_);

		bool const result = list_->insert( std::forward<UPAIR>(data) );

		if (list_->bytes() > maxSize_){
			flush();
			list_->clear();
			notifyLoader_();
		}

		return result;
	}

public:
	bool flush(){
		assert(list_);

		return flusher_ << *list_;
	}

private:
	template<typename T>
	struct loader_tag{};

	template<typename T>
	bool notifyLoader_(loader_tag<T>){
		assert(loader_);

		return loader_->refresh();
	}

	static bool notifyLoader_(loader_tag<nullptr_t>){
		return true;
	}

	bool notifyLoader_(){
		return notifyLoader_(loader_tag<LIST_LOADER>{});
	}

public:
	// Immutable Methods

	ObserverPair operator[](const StringRef &key) const{
		assert(list_);
		assert(!key.empty());
		return Pair::observer((*list_)[key]);
	}

	size_type size(bool const estimated = false) const{
		assert(list_);
		return list_->size(estimated);
	}

	size_t bytes() const{
		assert(list_);
		return list_->bytes();
	}

public:
	Iterator begin() const{
		assert(list_);
		return list_->begin();
	}

	Iterator end() const{
		assert(list_);
		return list_->end();
	}

	Iterator lowerBound(const StringRef &key) const{
		assert(list_);
		return list_->lowerBound(key);
	}

public:
	// Mutable Methods

	bool clear(){
		assert(list_);
		return list_->clear();
	}

	bool erase(const StringRef &key){
		assert(list_);
		assert(!key.empty());
		return list_->erase(key);
	}

private:
	LIST		*list_;
	FLUSH		flusher_;
	LIST_LOADER	*loader_;
	size_t		maxSize_;
};


} // namespace


#endif


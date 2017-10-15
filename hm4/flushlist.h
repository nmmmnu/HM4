#ifndef _FLUSH_LIST_H
#define _FLUSH_LIST_H

#include "decoratorlist.h"

#include "logger.h"

namespace hm4{


template <class LIST, class FLUSH, class LIST_LOADER = std::nullptr_t>
class FlushList : public DecoratorList<LIST, FlushList<LIST, FLUSH, LIST_LOADER> >{
	friend class IList<FlushList, LIST::MUTABLE>;

	static_assert(LIST::MUTABLE, "List must be mutable");

public:
	constexpr static size_t MAX_SIZE = 1 * 1024 * 1024;

private:
	template <class UFLUSH>
	FlushList(LIST &list, UFLUSH &&flusher, LIST_LOADER *loader, size_t const maxSize = MAX_SIZE) :
					DecoratorList<LIST, FlushList<LIST, FLUSH, LIST_LOADER> >(list),
						list_(list),
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
		flush_();
	}

private:
	template <class UPAIR>
	bool insertT_(UPAIR &&data){
		bool const result = list_.insert( std::forward<UPAIR>(data) );

		if (list_.bytes() > maxSize_){
			flush();
		}

		return result;
	}

public:
	bool flush(){
		bool const r = flush_();
		list_.clear();
		notifyLoader_();

		return r;
	}

	// Command pattern
	int command(bool const completeFlush){
		if (completeFlush)
			return flush();
		else
			return notifyLoader_();
	}

private:
	template<typename T>
	bool notifyLoader_(const T *){
		log__("Reloading data...");
		return loader_ && loader_->refresh();
	}

	static bool notifyLoader_(const std::nullptr_t *){
		return true;
	}

	bool notifyLoader_(){
		return notifyLoader_(loader_);
	}

	bool flush_(){
		log__("Flushing data...");
		return flusher_ << list_;
	}

private:
	LIST		&list_;
	FLUSH		flusher_;
	LIST_LOADER	*loader_;
	size_t		maxSize_;
};


} // namespace


#endif


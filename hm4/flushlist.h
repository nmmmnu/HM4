#ifndef _FLUSH_LIST_H
#define _FLUSH_LIST_H

#include "decoratorlist.h"

#include "logger.h"

namespace hm4{


template <class LIST, class FLUSH, class LIST_LOADER = std::nullptr_t>
class FlushList : public DecoratorList<LIST, FlushList<LIST, FLUSH, LIST_LOADER> >{
	static_assert(LIST::MUTABLE_TAG, "List must be mutable");
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
		flush();
	}

private:
	friend class IList<FlushList<LIST, FLUSH, LIST_LOADER>, true>;

	template <class UPAIR>
	bool insertT_(UPAIR &&data){
		bool const result = list_.insert( std::forward<UPAIR>(data) );

		if (list_.bytes() > maxSize_){
			flush();
			list_.clear();
			notifyLoader_();
		}

		return result;
	}

public:
	bool flush(){
		log__("Flushing data...");
		return flusher_ << list_;
	}

	// Command pattern
	int command(int = 0){
		return flush();
	}

private:
	template<typename T>
	struct loader_tag{};

	template<typename T>
	bool notifyLoader_(loader_tag<T>){
		log__("Reloading data...");
		return loader_ && loader_->refresh();
	}

	static bool notifyLoader_(loader_tag<std::nullptr_t>){
		return true;
	}

	bool notifyLoader_(){
		return notifyLoader_(loader_tag<LIST_LOADER>{});
	}

private:
	LIST		&list_;
	FLUSH		flusher_;
	LIST_LOADER	*loader_;
	size_t		maxSize_;
};


} // namespace


#endif


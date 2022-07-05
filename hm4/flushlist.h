#ifndef FLUSH_LIST_H_
#define FLUSH_LIST_H_

#include "multi/singlelist.h"

#include "flushlistbase.h"

namespace hm4{



template <class List, class Predicate, class Flusher, class ListLoader = std::nullptr_t>
class FlushList : public multi::SingleList<List>{
private:
	template <class UPredicate, class UFlusher>
	FlushList(List &list, UPredicate &&predicate, UFlusher &&flusher, ListLoader *loader) :
					multi::SingleList<List>(list),
						predicate_	(std::forward<UPredicate>(predicate)	),
						flusher_	(std::forward<UFlusher>(flusher)	),
						loader_		(loader					){}

public:
	using Allocator = typename multi::SingleList<List>::Allocator;

	template <class UPredicate, class UFlusher>
	FlushList(List &list, UPredicate &&predicate, UFlusher &&flusher, ListLoader &loader) :
					FlushList(list, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), &loader){}

	template <class UPredicate, class UFlusher>
	FlushList(List &list, UPredicate &&predicate, UFlusher &&flusher) :
					FlushList(list, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), nullptr){}

	~FlushList(){
		save_();
	}

	auto insert(	std::string_view const key, std::string_view const val,
			uint32_t const expires = 0, uint32_t const created = 0
			){

		return insert_(key, val, expires, created);
	}

	auto insert(Pair const &src){
		return insert_(src);
	}

	void flush(){
		save_();

		flushlist_impl_::clear(*list_, loader_);

	//	return true;
	}

	// Command pattern
	bool command(){
		return flush();
	}

private:
	template<typename ...Ts>
	auto insert_(Ts&&... ts){
		auto it = list_->insert(std::forward<Ts>(ts)...);

		if (predicate_(*list_))
			flush();

		return it;
	}

	void save_() const{
		log__("Save data...", "List record(s): ", list_->size(), "List size: ", list_->bytes());

		flushlist_impl_::save(*list_, flusher_);

		log__("Save done");
	}

private:
	using multi::SingleList<List>::list_;

	Predicate	predicate_;
	Flusher		flusher_;
	ListLoader	*loader_;
};


} // namespace


#endif


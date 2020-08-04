#ifndef BIN_LOG_LIST_H_
#define BIN_LOG_LIST_H_

#include "multi/singlelist.h"
#include "logger.h"


namespace hm4{


template <class List, class BinLogger>
class BinLogList : public multi::SingleList<List>{
public:
	using Allocator = typename multi::SingleList<List>::Allocator;

	template <class UBinLogger>
	BinLogList(List &list, UBinLogger &&binlogger) :
					multi::SingleList<List>(list),
						binlogger_	(std::forward<UBinLogger>(binlogger)	){}

	~BinLogList(){
		binlogger_.unlinkFile();
	}

	auto insert(	std::string_view const key, std::string_view const val,
			uint32_t const expires = 0, uint32_t const created = 0
			){

		return hm4::insert(*this, key, val, expires, created);
	}

	auto insert(Pair const &src){
		return hm4::insert(*this, src);
	}

	auto insert(typename Pair::smart_ptr::type<Allocator> &&newdata){
		binlogger_(*newdata);

		return list_->insert(std::move(newdata));
	}

	bool erase(std::string_view const key){
		assert(Pair::check(key));

		return this->insert(key, Pair::TOMBSTONE) != std::end(*list_);
	}

	bool clear(){
		bool const result = list_->clear();

		binlogger_.clear();

		return result;
	}

private:
	using		multi::SingleList<List>::list_;
	BinLogger	binlogger_;
};


} // namespace


#endif


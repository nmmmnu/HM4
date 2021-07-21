#ifndef BIN_LOG_LIST_H_
#define BIN_LOG_LIST_H_

#include "multi/singlelist.h"
#include "logger.h"


namespace hm4{

namespace binloglist_impl{

	// sfinae away d-tor.

	template <class BinLogger, bool>
	struct BinLogListBase{
		template <class UBinLogger>
		BinLogListBase(UBinLogger &&binlogger) : binlogger_(std::forward<UBinLogger>(binlogger)	){}

		BinLogger	binlogger_;
	};


	template <class BinLogger>
	struct BinLogListBase<BinLogger, true> : public BinLogListBase<BinLogger, false>{
		using BinLogListBase<BinLogger, false>::BinLogListBase;

		~BinLogListBase(){
			this->binlogger_.unlinkFile();
		}
	};

} // namespace binloglist_impl

template <class List, class BinLogger, bool UnlinkFile>
class BinLogList : public multi::SingleList<List>, binloglist_impl::BinLogListBase<BinLogger, UnlinkFile>{
public:
	using Allocator = typename multi::SingleList<List>::Allocator;

	template <class UBinLogger>
	BinLogList(List &list, UBinLogger &&binlogger) :
					multi::SingleList<List>(list),
					binloglist_impl::BinLogListBase<BinLogger, UnlinkFile>(std::forward<UBinLogger>(binlogger)){}

	auto insert(	std::string_view const key, std::string_view const val,
			uint32_t const expires = 0, uint32_t const created = 0
			){

		return insert_(key, val, expires, created);
	}

	auto insert(Pair const &src){
		return insert_(src);
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
	template<typename ...Ts>
	auto insert_(Ts&&... ts){
		auto it = list_->insert(std::forward<Ts>(ts)...);

		if (it == std::end(*list_))
			return it;

		binlogger_(*it);

		return it;
	}

private:
	using	multi::SingleList<List>::list_;
	using	binloglist_impl::BinLogListBase<BinLogger, UnlinkFile>::binlogger_;
};


} // namespace


#endif


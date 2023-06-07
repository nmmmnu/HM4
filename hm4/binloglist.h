#ifndef BIN_LOG_LIST_H_
#define BIN_LOG_LIST_H_

#include "multi/singlelist.h"


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

	bool erase_(std::string_view const key){
		assert(!key.empty());

		return insert(*this, key) != std::end(*list_);
	}

	bool clear(){
		bool const result = list_->clear();

		binlogger_.clear();

		return result;
	}

	template<class PFactory>
	auto insertF(PFactory &factory){
		auto it = list_->insertF(factory);

		if (it == std::end(*list_))
			return it;

		binlogger_(*it);

		return it;
	}

	void mutable_notify(const Pair *p, PairFactoryMutableNotifyMessage const &message){
		binlogger_(*p);

		return list_->mutable_notify(p, message);
	}

	constexpr void crontab() const{
		// no flush while read only

		list_->crontab();
	}

	void crontab(){
		binlogger_.flush();

		list_->crontab();
	}

private:
	using	multi::SingleList<List>::list_;
	using	binloglist_impl::BinLogListBase<BinLogger, UnlinkFile>::binlogger_;
};



} // namespace


#endif


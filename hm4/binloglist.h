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

	bool clear(){
		bool const result = list_->clear();

		binlogger_.clear();

		return result;
	}

	template<class PFactory>
	auto insertF(PFactory &factory){
		auto const result = list_->insertF(factory);

		// if (!result.ok)
		// 	return result;

		if (result.pair)
			binlogger_(*result.pair);

		return result;
	}

	InsertResult erase_(std::string_view const key){
		assert(!key.empty());

		auto result = list_->erase_(key);

		if (result.status == result.DELETED){
			PairBufferTombstone buffer;

			auto &pair = createTombstone__(key, buffer);

			binlogger_(pair);
		}else if (result.pair){
			binlogger_(*result.pair);
		}

		return result;
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
	// this will be on the stack, so need to be small-ish.
	static_assert(sizeof(PairBufferTombstone) < 2048);

	static const Pair &createTombstone__(std::string_view const key, PairBufferTombstone &buffer){
		Pair *pair = reinterpret_cast<Pair *>(buffer.data());

		auto const factory = PairFactory::Tombstone{ key };

		factory.create(pair);

		return *pair;
	}

private:
	using	multi::SingleList<List>::list_;
	using	binloglist_impl::BinLogListBase<BinLogger, UnlinkFile>::binlogger_;
};



} // namespace


#endif


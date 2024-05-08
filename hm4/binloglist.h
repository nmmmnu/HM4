#ifndef BIN_LOG_LIST_H_
#define BIN_LOG_LIST_H_

#include "multi/singlelist.h"

#include "ilist/makepair.h"
#include "ilist/txguard.h"


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

	alignas(Pair) char buffer_begin[ Pair::bytes(PairConf::TX_KEY_BEGIN, Pair::TOMBSTONE) ];
	alignas(Pair) char buffer_end  [ Pair::bytes(PairConf::TX_KEY_END,   Pair::TOMBSTONE) ];

	const Pair *pair_begin = makePairSystem( PairConf::TX_KEY_BEGIN, buffer_begin );
	const Pair *pair_end   = makePairSystem( PairConf::TX_KEY_END,   buffer_end );

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
			// this will be on the stack, so need to be small-ish.
			static_assert(sizeof(PairBufferTombstone) < 2048);

			PairBufferTombstone buffer;

			auto *pair = makePairTombstone(key, buffer);

			binlogger_(pair);
		}else if (result.pair){
			binlogger_(result.pair);
		}

		return result;
	}

	void mutable_notify(PairFactoryMutableNotifyMessage const &message){
		binlogger_(message.pair);

		return list_->mutable_notify(message);
	}

	constexpr void beginTX(){
		binlogger_(binloglist_impl::pair_begin);

		list_->beginTX();
	}

	constexpr void endTX(){
		binlogger_(binloglist_impl::pair_end);

		list_->endTX();
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


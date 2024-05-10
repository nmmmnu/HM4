#ifndef TX_LIST_H_
#define TX_LIST_H_

#include "multi/singlelist.h"

#include "ilist/makepair.h"



namespace hm4{



template <class List>
class TXList : public multi::SingleList<List>{
public:
	using Allocator = typename multi::SingleList<List>::Allocator;

	constexpr TXList(List &list) : multi::SingleList<List>(list){}

	~TXList(){
		if (tx_ == TX::BEGIN_EMIT)
			list->endTX();
	}

	bool clear(){
		return list_->clear();
	}

	template<class PFactory>
	auto insertF(PFactory &factory){
		lazyEmitTX_();

		return list_->insertF(factory);
	}

	InsertResult erase_(std::string_view const key){
		assert(!key.empty());

		lazyEmitTX_();

		return list_->erase_(key);
	}

	void mutable_notify(PairFactoryMutableNotifyMessage const &message){
		lazyEmitTX_();

		return list_->mutable_notify(message);
	}

	constexpr void beginTX(){
		tx_ = TX::BEGIN_LAZY;
	}

	constexpr void endTX(){
		list_->endTX();
	}

private:
	void lazyEmitTX_(){
		if (tx_ == TX::BEGIN_LAZY){
			list->beginTX();
			tx_ = TX::BEGIN_EMIT;
		}
	}

private:
	enum class TX{
		NONE,
		BEGIN_LAZY,
		BEGIN_EMIT
	};

	TX tx_ = TX::NONE;

};



} // namespace



#endif


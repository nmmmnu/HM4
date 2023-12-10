#ifndef MY_ILIST_UPDATE_IN_PLACE_H_
#define MY_ILIST_UPDATE_IN_PLACE_H_

#include "ilist.h"
#include "listcounter.h"

namespace hm4{

	// tryInsertHintAllocator
	//	To be used only inside Lists
	//	if possible, use hint, else do nothing.

	template<class Allocator, class PairFactory>
	constexpr bool tryUpdateInPlaceLC(Allocator const&allocator, const Pair *pair, PairFactory &factory, ListCounter &lc){
		if (! canInsertHintAllocatorF(allocator, pair, factory) )
			return false;

		auto const old_bytes = pair->bytes();

		proceedInsertHint_skipMutableNotify(pair, factory);

		auto const new_bytes = pair->bytes();

		lc.upd(old_bytes, new_bytes);

		return true;
	}

} // namespace hm4

#endif


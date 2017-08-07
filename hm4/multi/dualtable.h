#ifndef DUAL_TABLE_H_
#define DUAL_TABLE_H_

#include "ilist.h"

#include "dualiterator.h"


namespace hm4{
namespace multi{


template <class LIST1, class LIST2>
class DualTable : public IImmutableList<DualTable<LIST1,LIST2> >{
public:
	using Iterator = DualIterator<LIST1, LIST2>;

	using size_type = typename DualTable::size_type;

public:
	DualTable() = default;

	DualTable(const LIST1 &list1, const LIST2 &list2) :
					list1_( & list1),
					list2_( & list2){
	}

public:
	ObserverPair operator[](const StringRef &key) const{
		assert(list1_ && list2_);
		assert(!key.empty());

		const Pair &pair = (*list1_)[key];

		if (pair)
			return Pair::observer(pair);

		return Pair::observer((*list2_)[key]);
	}

	size_type size(bool const estimated = false) const{
		return estimated ? sizeEstimated_(estimated) : sizeReal_();
	}

	size_t bytes() const{
		assert(list1_ && list2_);

		return list1_->bytes() + list2_->bytes();
	}

public:
	Iterator begin() const{
		assert(list1_ && list2_);

		return Iterator(*list1_, *list2_, typename Iterator::begin_iterator{} );
	}

	Iterator end() const{
		assert(list1_ && list2_);

		return Iterator(*list1_, *list2_, typename Iterator::end_iterator{} );
	}

	Iterator lowerBound(const StringRef &key) const{
		assert(list1_ && list2_);

		return Iterator(*list1_, *list2_, key );
	}

private:
	size_type sizeEstimated_(bool const estimated) const{
		assert(list1_ && list2_);

		return list1_->size(estimated) + list2_->size(estimated);
	}

	size_type sizeReal_() const{
		// Slooooow....
		size_type count = 0;
		for(auto it = begin(); it != end(); ++it)
			++count;

		return count;
	}

private:
	const LIST1	*list1_ = nullptr;
	const LIST2	*list2_ = nullptr;
};


} // multi
} // namespace


#endif

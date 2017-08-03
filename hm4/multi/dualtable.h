#ifndef _DUAL_LIST_H
#define _DUAL_LIST_H

#include "ilist.h"

#include "dualiterator.h"


namespace hm4{
namespace multi{


template <class TABLE1, class TABLE2>
class DualTable : public IList<DualTable<TABLE1,TABLE2> >{
public:
	using Iterator = DualIterator<TABLE1, TABLE2>;

	using size_type = typename DualTable::size_type;

public:
	DualTable() = default;

	DualTable(const TABLE1 &table1, const TABLE2 &table2) :
					table1_( & table1),
					table2_( & table2){
	}

public:
	ObserverPair operator[](const StringRef &key) const{
		assert(table1_ && table2_);
		assert(!key.empty());

		const Pair &pair = (*table1_)[key];

		if (pair)
			return Pair::observer(pair);

		return Pair::observer((*table2_)[key]);
	}

	size_type size(bool const estimated = false) const{
		return estimated ? sizeEstimated_() : sizeReal_();
	}

	size_t bytes() const{
		assert(table1_ && table2_);

		return table1_->bytes() + table2_->bytes();
	}

public:
	Iterator begin() const{
		assert(table1_ && table2_);

		return Iterator(*table1_, *table2_, typename Iterator::begin_iterator{} );
	}

	Iterator end() const{
		assert(table1_ && table2_);

		return Iterator(*table1_, *table2_, typename Iterator::end_iterator{} );
	}

	Iterator lowerBound(const StringRef &key) const{
		assert(table1_ && table2_);

		return Iterator(*table1_, *table2_, key );
	}

private:
	size_type sizeEstimated_() const{
		assert(table1_ && table2_);

		return table1_->size(false) + table2_->size(false);
	}

	size_type sizeReal_() const{
		// Slooooow....
		size_type count = 0;
		for(auto it = begin(); it != end(); ++it)
			++count;

		return count;
	}

private:
	const TABLE1	*table1_ = nullptr;
	const TABLE2	*table2_ = nullptr;
};


} // multi
} // namespace


#endif

#ifndef _DUAL_LIST_H
#define _DUAL_LIST_H

#include "ilist.h"

#include "dualiterator.h"


namespace hm4{
namespace multi{


template <class TABLE1, class TABLE2>
class DualTable : public List<DualTable<TABLE1,TABLE2> >{
public:
	using Iterator = DualIterator<TABLE1, TABLE2>;

	using size_type = typename DualTable::size_type;

public:
	DualTable(const TABLE1 &table1, const TABLE2 &table2) :
					table1_(table1),
					table2_(table2){
	}

//	DualTable(DualTable &&other) = default;

public:
	const Pair &operator[](const StringRef &key) const{
		// precondition
		assert(!key.empty());
		// eo precondition

		const Pair &pair = table1_[key];

		if (pair)
			return pair;

		return table2_[key];
	}

	size_type size(bool const estimated = false) const{
		return estimated ? sizeEstimated_() : sizeReal_();
	}

	size_t bytes() const{
		return table1_.bytes() + table2_.bytes();
	}

public:
	Iterator begin() const{
		return Iterator(table1_, table2_, typename Iterator::begin_iterator{} );
	}

	Iterator end() const{
		return Iterator(table1_, table2_, typename Iterator::end_iterator{} );
	}

	Iterator lowerBound(const StringRef &key) const{
		return Iterator(table1_, table2_, key );
	}

private:
	size_type sizeEstimated_() const{
		return table1_.size(false) + table2_.size(false);
	}

	size_type sizeReal_() const{
		// Slooooow....
		size_type count = 0;
		for(const auto &p : *this){
			(void) p;
			++count;
		}

		return count;
	}

private:
	const TABLE1	&table1_;
	const TABLE2	&table2_;
};


} // multi
} // namespace


#endif

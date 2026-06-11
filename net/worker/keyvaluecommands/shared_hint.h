#ifndef SHARED_HINT_H_
#define SHARED_HINT_H_

namespace hm4{

	// insertHint family - standard insert, but uses hint, if possible
	//
	//	Pair is not assumed to be valid

	template<class List, class Factory>
	void insertHintVFactory(List &list, const Pair *pair, Factory &factory){
		using VBase = hm4::PairFactory::IFactory;

		// lost the type and skip one virtual call
		VBase &vbase = factory;

		if (pair && canInsertHintValSize(list, pair, factory.bytes() ))
			proceedInsertHint(list, const_cast<Pair *>(pair), vbase);
		else
			insert(list, vbase);
	}

}

#endif


#ifndef SHARED_HINT_H_
#define SHARED_HINT_H_

namespace hm4{

	template<class List, class Factory>
	void insertHintVFactory(const Pair *pair, List &list, Factory &factory){
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


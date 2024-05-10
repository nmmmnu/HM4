#ifndef BINLOG_REPLAY_H_
#define BINLOG_REPLAY_H_

namespace hm4{

	template <class List, class InputList>
	void binlogFileReplay(List &list, InputList const &inputList){
		for(auto it = std::begin(inputList); it != std::end(inputList); ++it){
		//	if (it->isSystemPair())
		//		continue;

			if (!it->isKeyEmpty())
				insert(list, *it);
		}
	}

} // namespace hm4

#endif


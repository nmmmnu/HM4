#ifndef BINLOG_REPLAY_H_
#define BINLOG_REPLAY_H_

namespace hm4{

	namespace binlog_replay_impl_{

		template <class InputList>
		auto checkReplayEnd(InputList const &inputList){
			auto end  = std::end(inputList);
			auto last = end;

			for(auto it = std::begin(inputList); it != std::end(inputList); ++it){
				if (!it->isSystemPair())
					continue;

				if (it->equals(hm4::PairConf::TX_KEY_BEGIN)){
					// we will ignore if we have two begin's one after the other.
					last = it;
					continue;
				}

				if (it->equals(hm4::PairConf::TX_KEY_END)){
					// we will ignore if we have two end's one after the other.
					last = end;
					continue;
				}
			}

			return last;
		}

	} // namespace binlog_replay_impl_

	template <class List, class InputList>
	void binlogFileReplay(List &list, InputList const &inputList){
		using namespace binlog_replay_impl_;

		auto const last = checkReplayEnd(inputList);

		for(auto it = std::begin(inputList); it != last; ++it){
			if (it->isSystemPair())
				continue;

			if (!it->isKeyEmpty())
				insert(list, *it);
		}
	}

} // namespace hm4

#endif


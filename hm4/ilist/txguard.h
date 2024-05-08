#ifndef TX_GUARD_H_
#define TX_GUARD_H_

namespace hm4{

	template<typename List>
	struct TXGuard{
		constexpr TXGuard(List &list) : list(list){
			list.beginTX();
		}

		TXGuard(TXGuard const &other) = delete;

		constexpr TXGuard(TXGuard &&other) : list(other.list){
			other.moved = true;
		};

		~TXGuard(){
			if (!moved)
				list.endTX();
		}

		constexpr void boom_(){
			moved = true;
		}

	private:
		List &list;
		bool moved = false;
	};

} // namespace hm4

#endif


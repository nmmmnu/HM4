#ifndef TX_GUARD_H_
#define TX_GUARD_H_

namespace hm4{

	struct TXGuard{
		template<typename List>
		constexpr TXGuard(List const &){
		}
	};

} // namespace hm4

#endif


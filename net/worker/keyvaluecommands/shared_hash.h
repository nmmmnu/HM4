#ifndef SHARED_HASH_H_
#define SHARED_HASH_H_

#include <string_view>

namespace hm4{

	constexpr bool isHKeyValid(size_t keyN, size_t subN){
		return hm4::Pair::isKeyValid(keyN + subN + 1 + 16);
	}

	constexpr bool isHKeyValid(std::string_view keyN, std::string_view subN){
		return isHKeyValid(keyN.size(), subN.size());
	}

	constexpr bool isHKeyValid(size_t keyN){
		return hm4::Pair::isKeyValid(keyN);
	}

	constexpr bool isHKeyValid(std::string_view keyN){
		return hm4::Pair::isKeyValid(keyN);
	}

} // namespace hm4

#endif


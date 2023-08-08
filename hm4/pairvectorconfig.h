#ifndef PAIR_VECTOR_CONFIG_H_
#define PAIR_VECTOR_CONFIG_H_

#include "pointer_iterator.h"

#include "hpair.h"

namespace hm4{

	namespace PairVectorConfig{
		struct KData {
			HPair::HKey		hkey;
			std::string_view	key;
		};

		struct Data{
			HPair::HKey		hkey	= 0;
			Pair			*pair	= nullptr;

			constexpr Data() = default;

			Data(Pair *pair) :
					hkey(HPair::SS::create(pair->getKey())),
					pair(pair){}

			int cmp(KData const kdata) const{
				return cmp(kdata.hkey, kdata.key);
			}

			constexpr Pair const &operator *() const{
				return *pair;
			}

		private:
			int cmp(HPair::HKey const hkey, std::string_view const key) const{
				return HPair::cmp(this->hkey, *this->pair, hkey, key);
			}
		};

		using const_ptr_iterator	= const Data *;
		using ptr_iterator		= Data *;

		using iterator			= pointer_iterator<const_ptr_iterator>;

	} // namespace PairVectorConfig

} // namespace hm4

#endif


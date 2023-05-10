#ifndef PAIR_VFACTORY_H_
#define PAIR_VFACTORY_H_

#include "smart_memcpy.h"

namespace hm4{
inline namespace version_4_00_00{
	namespace PairFactory{

		struct Reserve : IFactory{
			Reserve(std::string_view const key, size_t const val_size) :
							key		(key		),
							val_size	(val_size	){}

			constexpr std::string_view getKey() const final{
				return key;
			}

			constexpr uint32_t getCreated() const final{
				return 0;
			}

			constexpr size_t bytes() const final{
				return Pair::bytes(key.size(), val_size);
			}

			void createHint(Pair *pair) final{
				if (pair->getVal().size() != val_size){
					Pair::createInRawMemory<0,0,0,1>(pair, key, val_size, 0, 0);
					memset_(pair);
				}
			}

			void create(Pair *pair) final{
				Pair::createInRawMemory<1,0,1,1>(pair, key, val_size, 0, 0);
				memset_(pair);
			}

		private:
			static void memset_(Pair *pair){
				char *p = pair->getValC();
				memset(p, fill, pair->getVal().size());
			}

		private:
			std::string_view	key;
			size_t			val_size;

			constexpr static char fill = '\0';
		};



		struct SetSize : IFactory{
			constexpr SetSize(std::string_view const key, size_t val_size, const Pair *pair) :
					key		(key		),
					val_size	(val_size	),
					old_pair	(pair		){}

			[[nodiscard]]
			constexpr std::string_view getKey() const final{
				return key;
			}

			[[nodiscard]]
			constexpr uint32_t getCreated() const final{
				return 0;
			}

			[[nodiscard]]
			constexpr size_t bytes() const final{
				return Pair::bytes(key.size(), val_size);
			}

			void createHint(Pair *pair) final{
				Pair::createInRawMemory<0,0,0,1>(pair, key, val_size, 0, 0);
			}

			void create(Pair *pair) final{
				Pair::createInRawMemory<1,0,1,1>(pair, key, val_size, 0, 0);
				create_(pair);
			}

		private:
			void create_(Pair *pair) const{
				char *data = pair->getValC();

				if (old_pair){
					smart_memcpy(data, val_size, old_pair->getVal());
				}else{
					memset(data, '\0', val_size);
				}
			}

		private:
			std::string_view	key;
			size_t			val_size;
			const Pair		*old_pair;
		};

	} // namespace PairFactory
} // anonymous namespace
} // namespace

#endif


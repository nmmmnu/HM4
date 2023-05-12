#ifndef PAIR_VFACTORY_H_
#define PAIR_VFACTORY_H_

#include "smart_memcpy.h"

namespace hm4{
inline namespace version_4_00_00{
	namespace PairFactory{

		template<bool copy_value, bool same_size>
		struct IFactoryAction : IFactory{
			IFactoryAction(std::string_view const key, size_t const val_size, const Pair *old_pair) :
							key		(key		),
							val_size	(val_size	),
							old_pair	(old_pair	){}

			IFactoryAction(std::string_view const key, size_t const val_size) :
							IFactoryAction(key, val_size, nullptr){}

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
				if constexpr(same_size){
					// Same Size

					if (pair->getVal().size() != val_size){
						Pair::createInRawMemory<0,0,0,1>(pair, key, val_size, 0, 0);
					}else{
						// size OK
					}
				}else{
					// Standard

					Pair::createInRawMemory<0,0,0,1>(pair, key, val_size, 0, 0);
				}

				copyPair_(pair);

				return action(pair);
			}

			void create(Pair *pair) final{
				Pair::createInRawMemory<1,0,1,1>(pair, key, val_size, 0, 0);

				copyPair_(pair);

				return action(pair);
			}

		private:
			virtual void action(Pair *pair) = 0;

		private:
			void copyPair_(Pair *pair) const{
				if constexpr(copy_value){
					char *data = pair->getValC();

					if (old_pair){
						smart_memcpy(data, val_size, old_pair->getVal());
					}else{
						memset(data, '\0', val_size);
					}
				}
			}

		protected:
			std::string_view	key;
			size_t			val_size;
			const Pair		*old_pair;
		};



		struct Reserve : IFactoryAction<true, true>{
			using IFactoryAction::IFactoryAction;

			void action(Pair *) final{
			}
		};



		struct SetSize : IFactoryAction<true, false>{
			using IFactoryAction::IFactoryAction;

			void action(Pair *) final{
			}
		};

	} // namespace PairFactory
} // anonymous namespace
} // namespace

#endif


#ifndef PAIR_VFACTORY_H_
#define PAIR_VFACTORY_H_

#include "smart_memcpy.h"

#if defined(__clang__)
	#define constexpr_virtual /* constexpr */
#else
	#define constexpr_virtual constexpr
#endif

namespace hm4{
inline namespace version_4_00_00{
	namespace PairFactory{



		template<bool copy_value, bool same_size, typename Child>
		struct IFactoryAction : IFactory{
			constexpr IFactoryAction(std::string_view const key, size_t const val_size, const Pair *old_pair) :
							key		(key		),
							val_size	(val_size	),
							old_pair	(old_pair	){}

			constexpr IFactoryAction(std::string_view const key, size_t const val_size) :
							IFactoryAction(key, val_size, nullptr){}

			[[nodiscard]]
			constexpr_virtual std::string_view getKey() const final{
				return key;
			}

			[[nodiscard]]
			constexpr_virtual uint32_t getCreated() const final{
				return 0;
			}

			[[nodiscard]]
			constexpr_virtual size_t bytes() const final{
				return Pair::bytes(key.size(), val_size);
			}

			[[nodiscard]]
			constexpr_virtual bool valid() const final{
				return
					Pair::isKeyValid(key) &&
					Pair::isValValid(val_size)
				;
			}

			void createHint(Pair *pair) final{
				if constexpr(same_size){
					// Same Size

					if (pair->isOK() && pair->getVal().size() == val_size){
						// size OK, but init create + expire
						Pair::createInRawMemoryNone(pair, key, val_size, 0, 0);
					}else{
						Pair::createInRawMemory<Pair::CopyKey::N,Pair::CopyVal::N,Pair::MakeKey::N,Pair::MakeVal::Y>(pair, key, val_size, 0, 0);
						copyPair_(pair);
					}
				}else{
					{
						// Standard
						Pair::createInRawMemory<Pair::CopyKey::N,Pair::CopyVal::N,Pair::MakeKey::N,Pair::MakeVal::Y>(pair, key, val_size, 0, 0);
						copyPair_(pair);
					}
				}

				return action(pair);
			}

			void create(Pair *pair) final{
				Pair::createInRawMemory<Pair::CopyKey::Y,Pair::CopyVal::N,Pair::MakeKey::Y,Pair::MakeVal::Y>(pair, key, val_size, 0, 0);

				copyPair_(pair);

				return action(pair);
			}

		private:
			auto &crpt_(){
				return *reinterpret_cast<Child *>(this);
			}

			const auto &crpt_() const{
				return *reinterpret_cast<const Child *>(this);
			}

		private:
			void action(Pair *pair){
				crpt_().action(pair);
			}

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



		struct Reserve : IFactoryAction<true, true, Reserve>{
			using IFactoryAction<true, true, Reserve>::IFactoryAction;

			constexpr void action(Pair *) const{
			}
		};



	//	struct SetSize : IFactoryAction<true, false, SetSize>{
	//		using IFactoryAction<true, false, SetSize>::IFactoryAction;
	//
	//		constexpr void action(Pair *) const{
	//		}
	//	};

	} // namespace PairFactory
} // anonymous namespace
} // namespace

#endif


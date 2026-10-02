#ifndef DECORATOR_List_H_
#define DECORATOR_List_H_

#include "ilist.h"

#include <cassert>

namespace hm4::multi{

	namespace single_list_impl_{

		template <class List>
		struct SingleListBase{
			using iterator		= typename List::iterator;

			using size_type		= typename List::size_type;
			using difference_type	= typename List::difference_type;

		public:
			SingleListBase(List &list) : list_(& list){}

		public:
			// Immutable Methods

			size_type size() const{
				return list_->size();
			}

			constexpr auto empty() const{
				// unlike CollectionList,
				// if list_->empty() is constexpr,
				// the optimizer will remove it

				return list_->empty();
			}

			auto const &mutable_list() const{
				return list_->mutable_list();
			}

			size_t bytes() const{
				return list_->bytes();
			}

			template<typename Command>
			constexpr auto chain(Command const &cmd) const{
				return list_->chain(cmd);
			}

			template<typename Command>
			constexpr auto chainMut(Command const &cmd){
				return list_->chainMut(cmd);
			}

			constexpr void crontab() const{
				list_->crontab();
			}

		public:
			iterator begin() const{
				return list_->begin();
			}

			iterator end() const{
				return list_->end();
			}

			iterator    find     (std::string_view const key) const{
				return list_->find(key);
			}

			const Pair *getPair___(std::string_view const key) const{
				return list_->getPair___(key);
			}

			const Pair *getPair___(std::string_view const key, const Pair *best) const{
				return list_->getPair___(key, best);
			}

		public:
			template<
				typename T1 = List,
				decltype(std::declval<T1>().rbegin(), 0) = 0
			>
			auto rbegin() const{
				return list_->rbegin();
			}

			template<
				typename T1 = List,
				decltype(std::declval<T1>().rend(), 0) = 0
			>
			auto rend() const{
				return list_->rend();
			}

			template<
				bool B,
				typename T1 = List,
				decltype(std::declval<T1>().rfind("", std::bool_constant<B>{}), 0) = 0
			>
			auto rfind(std::string_view const key, std::bool_constant<B> const exact) const{
				return list_->rfind(key, exact);
			}

		protected:
			List	*list_;
		};

	} // namespace single_list_impl_



	template<class List, class = std::void_t<> >
	struct SingleList : public single_list_impl_::SingleListBase<List>{
		using single_list_impl_::SingleListBase<List>::SingleListBase;
	};



	template<class List>
	struct SingleList<List, std::void_t<typename List::Allocator> > : public single_list_impl_::SingleListBase<List>{
		using single_list_impl_::SingleListBase<List>::SingleListBase;

		using Allocator = typename List::Allocator;

		const auto &getAllocator() const{
			return list_->getAllocator();
		}

		auto &getAllocator(){
			return list_->getAllocator();
		}

	public:
		// Mutable Methods

		bool clear(){
			return list_->clear();
		}

		auto erase___(std::string_view const key){
			// better Pair::check(key), but might fail because of the caller.
			assert(!key.empty());

			return hm4::erase(*list_, key);
		}

		template<class PFactory>
		auto insertF(PFactory &factory){
			return list_->insertF(factory);
		}

		constexpr void mutable_notify( PairFactoryMutableNotifyMessage const &message){
			return list_->mutable_notify(message);
		}

		constexpr void crontab(){
			list_->crontab();
		}

		constexpr void crontab() const{
			list_->crontab();
		}

	protected:
		using single_list_impl_::SingleListBase<List>::list_;
	};

} // namespace



#endif

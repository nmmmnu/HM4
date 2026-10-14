#ifndef TIMER_LIST_H_
#define TIMER_LIST_H_

#include "multi/singlelist.h"
#include "myhistogramtimer.h"
#include "timerlistchain.h"

namespace hm4{
	namespace timer_list_impl_{

		template <class List, uint64_t ID>
		struct TimerListBase : public multi::SingleList<List>{
			using Base = multi::SingleList<List>;

			using Base::SingleList;

		public:
			auto begin() const{
				auto const guard = timerRead_();

				return list_->begin();
			}

			auto find(std::string_view const key) const{
				auto const guard = timerRead_();

				return list_->find(key);
			}

			const Pair *getPair___(std::string_view const key) const{
				auto const guard = timerRead_();

				return list_->getPair___(key);
			}

			const Pair *getPair___(std::string_view const key, const Pair *best) const{
				auto const guard = timerRead_();

				return list_->getPair___(key, best);
			}

		public:
			using Base::chain;

			constexpr auto chain(chain::get_histogram<ID> const &) const -> my_histogram_timer::Timer *{
				return & timerRead_;
			}

		protected:
			mutable my_histogram_timer::Timer	timerRead_;

			using Base::list_;
		};
	} // timerlist_impl_



	template<class List, uint64_t ID, class = std::void_t<> >
	struct TimerList : public timer_list_impl_::TimerListBase<List, ID>{
		using Base = timer_list_impl_::TimerListBase<List, ID>;

		using Base::TimerListBase;
	};



	template<class List, uint64_t ID>
	class TimerList<List, ID, std::void_t<typename List::Allocator> > : public timer_list_impl_::TimerListBase<List, ID>{
		using Base = timer_list_impl_::TimerListBase<List, ID>;

	public:
		using Base::TimerListBase;

		using Allocator = typename Base::Allocator;

	//	using Base::USE_CRONTAB;

	public:
		template<class PFactory>
		auto insertF(PFactory &factory){
			auto const guard = timerWrite_();

			auto const result = list_->insertF(factory);

			return result;
		}

		InsertResult erase___(std::string_view const key){
			assert(!key.empty());

			auto const guard = timerWrite_();

			auto result = list_->erase___(key);

			return result;
		}

	public:
		using Base::chain;

		constexpr auto chain(chain::get_histogram<ID + 0> const &) const -> my_histogram_timer::Timer *{
			return & timerRead_;
		}

		constexpr auto chain(chain::get_histogram<ID + 1> const &) const -> my_histogram_timer::Timer *{
			return & timerWrite_;
		}

	private:
		mutable my_histogram_timer::Timer	timerWrite_;

		using Base::timerRead_;
		using Base::list_;
	};

} // namespace

#endif


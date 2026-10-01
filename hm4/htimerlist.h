#ifndef BIN_LOG_LIST_H_
#define BIN_LOG_LIST_H_

#include "multi/singlelist.h"
#include "myhistogramtimer.h"

namespace hm4{
	namespace chain{
		struct log_histogram{
			char		mode	= ANY;
			uint64_t	id	= 0;

			constexpr static char ANY	= 'A';
			constexpr static char READ	= 'R';
			constexpr static char WRITE	= 'W';
		};
	}

	namespace htimer_list_impl_{
		constexpr size_t		BAR_MAX_WIDTH	= 14;
		constexpr std::string_view	BAR_FULL	= "#";
		constexpr std::string_view	BAR_EMPTY	= " ";
		constexpr Logger::Level		LOG_LEVEL	= Logger::DEBUG;

		inline void logHistogram(my_histogram_timer::Timer const &h, std::string_view msg){
			auto const result = h.get(BAR_MAX_WIDTH);

			logger<LOG_LEVEL>() << msg << result.total << "events";

			for(size_t i = 0; i < h.size(); ++i){
				auto const &row = result.events[i];

				char barBuffer[BAR_MAX_WIDTH * 4 + 1]; // because of UTF8 * 4
				char *ptr = barBuffer;

				for (size_t b = 0; b < row.bars; ++b){
					auto const bar = BAR_FULL;

					memcpy(ptr, bar.data(), bar.size());
					ptr += bar.size();
				}

				for (size_t b = row.bars; b < BAR_MAX_WIDTH; ++b){
					auto const bar = BAR_EMPTY;

					memcpy(ptr, bar.data(), bar.size());
					ptr += bar.size();
				}

				*ptr = '\0';

				if (!row.value)
					continue;

				constexpr const char *FMT_MASK   = "[ {} - {} ) {:10} {:8.4} {:8.4} {}";

				logger_fmt<LOG_LEVEL>(FMT_MASK,
				//	row.id,

					row.labelF.data(),
					row.labelT.data(),

					(size_t) row.value,

					row.pct,
					row.pctCumulative,

					barBuffer
				);
			}
		}

		template<bool B>
		void logHistogram(my_histogram_timer::Timer const &h){
			if constexpr(B)
				logHistogram(h, "Write"		);
			else
				logHistogram(h, "Read"		);
		}



		template <class List>
		struct HTimerListBase : public multi::SingleList<List>{
			using Base = multi::SingleList<List>;

			HTimerListBase(List &list, uint64_t id = 0) :
								Base(list),
								id_(id){}

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

			constexpr void chain(chain::log_histogram const &a) const{
				if (id_ == a.id){
					if (a.mode == a.ANY || a.mode == a.READ)
						htimer_list_impl_::logHistogram<0>(timerRead_);
				}
			}

		protected:
			mutable my_histogram_timer::Timer	timerRead_;
			uint64_t				id_;

			using multi::SingleList<List>::list_;
		};
	} // htimerlist_impl_



	template<class List, class = std::void_t<> >
	struct HTimerList : public htimer_list_impl_::HTimerListBase<List>{
		using Base = htimer_list_impl_::HTimerListBase<List>;

		using Base::HTimerListBase;
	};



	template<class List>
	struct HTimerList<List, std::void_t<typename List::Allocator> > : public htimer_list_impl_::HTimerListBase<List>{
		using Base = htimer_list_impl_::HTimerListBase<List>;

		using Base::HTimerListBase;

		using Allocator = typename List::Allocator;

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

		constexpr void chain(chain::log_histogram const &a) const{
			if (id_ == a.id){
				if (a.mode == a.ANY || a.mode == a.READ)
					htimer_list_impl_::logHistogram<0>(timerRead_);

				if (a.mode == a.ANY || a.mode == a.WRITE)
					htimer_list_impl_::logHistogram<1>(timerWrite_);
			}
		}

	private:
		mutable my_histogram_timer::Timer	timerWrite_;

		using htimer_list_impl_::HTimerListBase<List>::timerRead_;
		using htimer_list_impl_::HTimerListBase<List>::list_;
		using htimer_list_impl_::HTimerListBase<List>::id_;
	};

} // namespace

#endif


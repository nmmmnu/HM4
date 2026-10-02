#ifndef TIMER_LIST_H_
#define TIMER_LIST_H_

#include "multi/singlelist.h"
#include "myhistogramtimer.h"

namespace hm4{
	namespace chain{
		template<uint64_t ID>
		struct log_histogram{
			char		mode	= ANY;

			constexpr static char ANY	= 'A';
			constexpr static char READ	= 'R';
			constexpr static char WRITE	= 'W';
		};
	}

	namespace timer_list_impl_{
		constexpr size_t		BAR_MAX_WIDTH	= 14;
		constexpr std::string_view	BAR_FULL	= "#";
		constexpr std::string_view	BAR_EMPTY	= " ";
		constexpr Logger::Level		LOG_LEVEL	= Logger::DEBUG;

		inline void logHistogram(my_histogram_timer::Timer const &h, uint64_t const id, std::string_view msg){
			auto const result = h.get(BAR_MAX_WIDTH);

			logger<LOG_LEVEL>() << "ID" << id << msg << result.total << "events";

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
		void logHistogram(my_histogram_timer::Timer const &h, uint64_t const id){
			if constexpr(B)
				logHistogram(h, id, "Write"	);
			else
				logHistogram(h, id, "Read"	);
		}



		template <class List, uint64_t ID>
		struct TimerListBase : public multi::SingleList<List>{
			using Base = multi::SingleList<List>;

			using Base::SingleList;

			constexpr static bool USE_CRONTAB = false;

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

			constexpr void chain(chain::log_histogram<ID> const &a) const{
				if (a.mode == a.ANY || a.mode == a.READ)
					timer_list_impl_::logHistogram<0>(timerRead_, ID);
			}

			using Base::crontab;

			constexpr void crontab(){
				crontab_();

				list_->crontab();
			}

			constexpr void crontab() const{
				if constexpr(USE_CRONTAB)
					crontab_();

				list_->crontab();
			}

		private:
			constexpr void crontab_(){
				timer_list_impl_::logHistogram<0>(timerRead_, ID);
			}

		protected:
			mutable my_histogram_timer::Timer	timerRead_;

			using multi::SingleList<List>::list_;
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

		using Base::USE_CRONTAB;

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

		constexpr void chain(chain::log_histogram<ID> const &a) const{
			if (a.mode == a.ANY || a.mode == a.READ)
				timer_list_impl_::logHistogram<0>(timerRead_,  ID);

			if (a.mode == a.ANY || a.mode == a.WRITE)
				timer_list_impl_::logHistogram<1>(timerWrite_, ID);
		}

		using Base::crontab;

		constexpr void crontab(){
			crontab_();

			list_->crontab();
		}

		constexpr void crontab() const{
			if constexpr(USE_CRONTAB)
				crontab_();

			list_->crontab();
		}

	private:
		constexpr void crontab_(){
			timer_list_impl_::logHistogram<0>(timerRead_,  ID);
			timer_list_impl_::logHistogram<1>(timerWrite_, ID);
		}


	private:
		mutable my_histogram_timer::Timer	timerWrite_;

		using timer_list_impl_::TimerListBase<List, ID>::timerRead_;
		using timer_list_impl_::TimerListBase<List, ID>::list_;
	};

} // namespace

#endif


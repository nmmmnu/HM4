#ifndef TIMER_LIST_H_
#define TIMER_LIST_H_

#include "multi/singlelist.h"
#include "myhistogramtimer.h"
#include "timerlistchain.h"

namespace hm4{
	namespace timer_list_impl_{
		constexpr size_t		BAR_MAX_WIDTH	= 14;

		constexpr std::string_view	BAR_FULL	= "#####" "#####" "#####" "#####";
	//	constexpr std::string_view	BAR_FULL	= "▪▪▪▪▪" "▪▪▪▪▪" "▪▪▪▪▪" "▪▪▪▪▪";
		constexpr std::string_view	BAR_EMPTY	= "     " "     " "     " "     ";

		constexpr Logger::Level		LOG_LEVEL	= Logger::DEBUG;

		inline void logHistogram(my_histogram_timer::Timer const &h, uint64_t const id, std::string_view msg){
			auto const result = h.get(BAR_MAX_WIDTH);

			logger<LOG_LEVEL>() << "ID" << id << msg << result.total << "events";

			for(size_t i = 0; i < h.size(); ++i){
				auto const &row = result.events[i];

				if (!row.value)
					continue;

				logger_fmt<LOG_LEVEL>(
					FMT_COMPILE("[ {} - {} ) {:10} {:8.2f} {:8.2f} {:.{}}{:.{}}"),
					row.labelF.data(),
					row.labelT.data(),

					(size_t) row.value,

					row.pct,
					row.pctCumulative,

					BAR_FULL ,	row.bars,
					BAR_EMPTY,	BAR_MAX_WIDTH - row.bars
				);
			}
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

			constexpr void chain(chain::log_histogram<ID> const &) const{
				// we proceed anyway
				logHistogram(timerRead_,  ID, "Read");
			}

			constexpr void chain(chain::reset_histogram<ID> const &) const{
				// we proceed anyway
				timerRead_.clear();
			}

			constexpr auto chain(chain::get_histogram<ID> const &a) const -> const my_histogram_timer::Timer *{
				using _ = chain::get_histogram<ID>;

				switch(a.mode){
				case _::READ	: return & timerRead_;
				default		: return nullptr;
				}
			}

			using Base::crontab;

			constexpr void crontab(){
				crontab_();

				list_->crontab();
			}

			constexpr void crontab() const{
				crontab_();

				list_->crontab();
			}

		private:
			constexpr void crontab_() const{
				if constexpr(USE_CRONTAB){
					logHistogram(timerRead_, ID, "Read");
				}
			}

		protected:
			mutable my_histogram_timer::Timer	timerRead_;

			using Base::list_;
		};
	} // timerlist_impl_



	using timer_list_impl_::logHistogram;



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
				logHistogram(timerRead_,  ID, "Read");

			if (a.mode == a.ANY || a.mode == a.WRITE)
				logHistogram(timerWrite_, ID, "Write");
		}

		constexpr void chain(chain::reset_histogram<ID> const &a) const{
			if (a.mode == a.ANY || a.mode == a.READ)
				timerRead_ .clear();

			if (a.mode == a.ANY || a.mode == a.WRITE)
				timerWrite_.clear();
		}

		constexpr auto chain(chain::get_histogram<ID> const &a) const -> const my_histogram_timer::Timer *{
			using _ = chain::get_histogram<ID>;

			switch(a.mode){
			case _::READ	: return & timerRead_;
			case _::WRITE	: return & timerWrite_;
			default		: return nullptr;
			}
		}

		using Base::crontab;

		constexpr void crontab(){
			crontab_();

			list_->crontab();
		}

		constexpr void crontab() const{
			crontab_();

			list_->crontab();
		}

	private:
		constexpr void crontab_(){
			if constexpr(USE_CRONTAB){
				logHistogram(timerRead_,  ID, "Read" );
				logHistogram(timerWrite_, ID, "Write");
			}
		}

	private:
		mutable my_histogram_timer::Timer	timerWrite_;

		using Base::timerRead_;
		using Base::list_;
	};

} // namespace

#endif


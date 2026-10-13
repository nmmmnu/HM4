#include "base.h"
#include "timerlist.h"
#include <algorithm>	// accumulate



namespace net::worker::commands::Latency{

	namespace latency_impl_{

		constexpr size_t		BAR_MAX_WIDTH	= 14;
		constexpr std::string_view	BAR_FULL	= "#####" "#####" "#####" "#####";
	//	constexpr std::string_view	BAR_FULL	= "▪▪▪▪▪" "▪▪▪▪▪" "▪▪▪▪▪" "▪▪▪▪▪";
		constexpr std::string_view	BAR_EMPTY	= "     " "     " "     " "     ";

		constexpr std::string_view	FMT_MASK	= "[ {} - {} ) {:10} {:8.2f} {:8.2f} {:.{}}{:.{}}"	;
		constexpr std::string_view	FMT_MASK_LN	= "[ {} - {} ) {:10} {:8.2f} {:8.2f} {:.{}}{:.{}}" "\n"	;

		constexpr char			SUB_DB[2]	= {'D', 'd'};
		constexpr char			SUB_CMD[2]	= {'C', 'c'};

		template<uint64_t id, class DBAdapter>
		auto getHistogramDB_(bool rw, DBAdapter const &db) -> my_histogram_timer::Timer *{
			using getH = hm4::chain::get_histogram<id>;

			auto const hrw = rw ? getH::WRITE : getH::READ;

			auto *hist = db->chain(getH{ hrw });

			if constexpr(std::is_same_v<std::remove_cv_t<decltype(hist)>, my_histogram_timer::Timer *>){
				return hist;
			}else{
				return nullptr;
			}
		}

		template<bool MUTABLE, class DBAdapter>
		auto getHistogramDB(uint64_t id, bool rw, DBAdapter const &db) -> my_histogram_timer::Timer *{
			if constexpr(MUTABLE){
				switch(id){
				case 0:  return getHistogramDB_<0>(rw, db);
				case 1:  return getHistogramDB_<1>(rw, db);
				case 2:  return getHistogramDB_<2>(rw, db);
				default: return nullptr;
				}
			}else{
				switch(id){
				case 0:  return getHistogramDB_<0>(rw, db);
				default: return nullptr;
				}
			}
		}

		template<bool MUTABLE, class DBAdapter, typename F>
		void forEachDB(DBAdapter const &db, F functor){
			std::string_view const sys = "DB";

			auto f = [&](uint64_t id, bool rw){
				if (auto *h = getHistogramDB<MUTABLE>(id, rw, db); h)
					functor(*h, sys, id, rw);
			};

			bool const R = 0;
			bool const W = 1;


			if constexpr(!MUTABLE){
				f(0, R);
			}else{
				f(0, R);
				f(0, W);

				f(1, R);
				f(1, W);

				f(2, R);
				f(2, W);
			}
		}

		template<typename F>
		void forEachCMD(OutputBlob &blob, F functor){
			std::string_view const sys = "CMD";

			functor(blob.getTimerCmd(), sys, 0, false);
		}

		template<bool MUTABLE, class DBAdapter, typename F>
		void forEachDB(DBAdapter const &db, OutputBlob &blob, F functor){
			// proceed for subsystem DB
			forEachDB<DBAdapter::MUTABLE>(db, functor);

			// proceed for subsystem Cmd
			forEachCMD(blob, functor);
		}
	} // namespace latency_impl_



	template<class Protocol, class DBAdapter>
	struct LATENCYCOUNTERS : BaseCommandRO<Protocol,DBAdapter>{

		LATENCYCOUNTERS() : BaseCommandRO<Protocol,DBAdapter>("LATENCYCOUNTERS", std::begin(cmd__), std::end(cmd__)){}

		// LATENCYCOUNTERS subsystem object_id R/W
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const subsystem = p[1][0];

			using namespace latency_impl_;

			switch(subsystem){
			case SUB_DB[0]:
			case SUB_DB[1]:
				{
					auto const  id   = from_string<uint64_t>(p[2]);
					auto const  rw   = p[3] == "w" || p[2] == "W";
					const auto *hist = getHistogramDB<DBAdapter::MUTABLE>(id, rw, db);

					return process__(id, rw, hist, result, blob);
				}

			case SUB_CMD[0]:
			case SUB_CMD[1]:
				{
					auto const id   = 0;
					auto const rw   = false;
					const auto hist = blob.getTimerCmd();

					return process__(id, rw, hist, result, blob);
				}

			default:
				// no subsystem selected
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		static void process__(uint64_t id, bool rw, const my_histogram_timer::Timer *hist, Result<Protocol> &result, OutputBlob &blob){
			if (!hist)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			else
				return process__(id, rw, *hist, result, blob);
		}

		static void process__(uint64_t , bool , const my_histogram_timer::Timer &hist, Result<Protocol> &result, OutputBlob &blob){

			auto &container  = blob.construct<OutputBlob::SmallContainer>();
			auto &bcontainer = blob.construct<OutputBlob::SmallBufferContainer>();

			for(auto const &num : hist){
				bcontainer.push_back();

				container.push_back(to_string(num, bcontainer.back()));
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"latencycounters",	"LATENCYCOUNTERS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct LATENCYCOUNT : BaseCommandRO<Protocol,DBAdapter>{

		LATENCYCOUNT() : BaseCommandRO<Protocol,DBAdapter>("LATENCYCOUNT", std::begin(cmd__), std::end(cmd__)){}

		// LATENCYCOUNT subsystem object_id R/W
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const subsystem = p[1][0];

			using namespace latency_impl_;

			switch(subsystem){
			case SUB_DB[0]:
			case SUB_DB[1]:
				{
					auto const  id   = from_string<uint64_t>(p[2]);
					auto const  rw   = p[3] == "w" || p[2] == "W";
					const auto *hist = getHistogramDB<DBAdapter::MUTABLE>(id, rw, db);

					return process__(id, rw, hist, result);
				}

			case SUB_CMD[0]:
			case SUB_CMD[1]:
				{
					auto const id   = 0;
					auto const rw   = false;
					const auto hist = blob.getTimerCmd();

					return process__(id, rw, hist, result);
				}

			default:
				// no subsystem selected
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		static void process__(uint64_t id, bool rw, const my_histogram_timer::Timer *hist, Result<Protocol> &result){
			if (!hist)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			else
				return process__(id, rw, *hist, result);
		}

		static void process__(uint64_t, bool, const my_histogram_timer::Timer &hist, Result<Protocol> &result){
			uint64_t const sum = std::accumulate(std::begin(hist), std::end(hist), uint64_t{ 0 });

			return result.set(sum);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"latencycount",	"LATENCYCOUNT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct LATENCYHUMAN : BaseCommandRO<Protocol,DBAdapter>{

		LATENCYHUMAN() : BaseCommandRO<Protocol,DBAdapter>("LATENCYHUMAN", std::begin(cmd__), std::end(cmd__)){}

		// LATENCYHUMAN subsystem object_id R/W
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const subsystem = p[1][0];

			using namespace latency_impl_;

			switch(subsystem){
			case SUB_DB[0]:
			case SUB_DB[1]:
				{
					auto const  id   = from_string<uint64_t>(p[2]);
					auto const  rw   = p[3] == "w" || p[2] == "W";
					const auto *hist = getHistogramDB<DBAdapter::MUTABLE>(id, rw, db);

					return process__(id, rw, hist, result, blob);
				}

			case SUB_CMD[0]:
			case SUB_CMD[1]:
				{
					auto const id   = 0;
					auto const rw   = false;
					const auto hist = blob.getTimerCmd();

					return process__(id, rw, hist, result, blob);
				}

			default:
				// no subsystem selected
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		static void process__(uint64_t id, bool rw, const my_histogram_timer::Timer *hist, Result<Protocol> &result, OutputBlob &blob){
			if (!hist)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			else
				return process__(id, rw, *hist, result, blob);
		}

		static void process__(uint64_t, bool, const my_histogram_timer::Timer &hist, Result<Protocol> &result, OutputBlob &blob){
			auto &buffer  = blob.allocate<std::array<char, BUFFER_SIZE> >();

			      auto *ptr = buffer.data();
			const auto *end = buffer.data() + buffer.size();

			auto const hresult = hist.get(BAR_MAX_WIDTH);

			for(size_t i = 0; i < hist.size(); ++i){
				auto const &row = hresult.events[i];

			//	if (!row.value)
			//		continue;

				size_t const remaining = static_cast<size_t>(end - ptr);

				auto const res = fmt::format_to_n(
					ptr,
					remaining,
					FMT_COMPILE(latency_impl_::FMT_MASK_LN),

					row.labelF,
					row.labelT,

					static_cast<size_t>(row.value),

					row.pct,
					row.pctCumulative,

					BAR_FULL,	row.bars,
					BAR_EMPTY,	BAR_MAX_WIDTH - row.bars
				);

				if (res.size > remaining)
					return result.set(ERROR_MESSAGE);

				ptr = res.out;
			}

			return result.set(
				std::string_view{
					buffer.data(),
					static_cast<size_t>(ptr - buffer.data())
				}
			);
		}

	private:
		constexpr static size_t			BUFFER_SIZE	= 2048; // 1910 when ASCII;

		constexpr static std::string_view	ERROR_MESSAGE	= "INTERNAL ERROR IN LATENCYHUMAN, PLEASE REPORT A BUG!";

	private:
		constexpr static size_t			BAR_MAX_WIDTH	= latency_impl_::BAR_MAX_WIDTH	;
		constexpr static std::string_view	BAR_FULL	= latency_impl_::BAR_FULL	;
		constexpr static std::string_view	BAR_EMPTY	= latency_impl_::BAR_EMPTY	;

	private:
		constexpr inline static std::string_view cmd__[] = {
			"latencyhuman",	"LATENCYHUMAN"
		};
	};



	template<class Protocol, class DBAdapter>
	struct LATENCYLOG : BaseCommandRO<Protocol,DBAdapter>{

		LATENCYLOG() : BaseCommandRO<Protocol,DBAdapter>("LATENCYLOG", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace latency_impl_;

			if (p.size() != 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_0);

			auto f = [](auto const &hist, std::string_view sys, uint64_t id, bool rw){
				logHistogram__(hist, sys, id, rw);
			};

			logger<LOG_LEVEL>() << "Dump all latency histograms:";

			forEachDB<DBAdapter::MUTABLE>(db, blob, f);

			return result.set();
		}

		static void logHistogram__(my_histogram_timer::Timer const &h, std::string_view sys, uint64_t const id, bool rw){
			std::string_view const msg = rw ? "Write" : "Read";

			auto const result = h.get(BAR_MAX_WIDTH);

			logger<LOG_LEVEL>() << "System" << sys << "ID" << id << msg << result.total << "events";
			logger<LOG_LEVEL>() << "--begin--";

			for(size_t i = 0; i < h.size(); ++i){
				auto const &row = result.events[i];

				if (!row.value)
					continue;

				logger_fmt<LOG_LEVEL>(
					FMT_COMPILE(latency_impl_::FMT_MASK),
					row.labelF.data(),
					row.labelT.data(),

					(size_t) row.value,

					row.pct,
					row.pctCumulative,

					BAR_FULL ,	row.bars,
					BAR_EMPTY,	BAR_MAX_WIDTH - row.bars
				);
			}

			logger<LOG_LEVEL>() << "---end---";
		}

	private:
		constexpr static size_t			BUFFER_SIZE	= 2048; // 1910 when ASCII;

		constexpr static Logger::Level		LOG_LEVEL	= Logger::DEBUG;

	private:
		constexpr static size_t			BAR_MAX_WIDTH	= latency_impl_::BAR_MAX_WIDTH	;
		constexpr static std::string_view	BAR_FULL	= latency_impl_::BAR_FULL	;
		constexpr static std::string_view	BAR_EMPTY	= latency_impl_::BAR_EMPTY	;

	private:
		constexpr inline static std::string_view cmd__[] = {
			"latencylog",	"LATENCYLOG"
		};

	};



	template<class Protocol, class DBAdapter>
	struct LATENCYRESET : BaseCommandRO<Protocol,DBAdapter>{

		LATENCYRESET() : BaseCommandRO<Protocol,DBAdapter>("LATENCYRESET", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() != 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_0);

			using namespace latency_impl_;

			auto f = [](auto &hist, std::string_view, uint64_t, bool){
				hist.clear();
			};

			forEachDB<DBAdapter::MUTABLE>(db, blob, f);

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"latencyreset",	"LATENCYRESET"
		};

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "latency";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				LATENCYCOUNTERS	,
				LATENCYCOUNT	,
				LATENCYHUMAN	,
				LATENCYLOG	,
				LATENCYRESET
			>(pack);
		}
	};



} // namespace



#include "base.h"
#include "timerlist.h"



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
		auto getHistogramDB_(DBAdapter const &db) -> my_histogram_timer::Timer *{
			auto *hist = db->chain( hm4::chain::get_histogram<id>{} );

			if constexpr(std::is_same_v<std::remove_cv_t<decltype(hist)>, my_histogram_timer::Timer *>){
				return hist;
			}else{
				return nullptr;
			}
		}

		template<class DBAdapter>
		auto getHistogramDB(uint64_t id, DBAdapter const &db) -> my_histogram_timer::Timer *{
			if constexpr(DBAdapter::MUTABLE){
				switch(id){
				case  0 + 0:  return getHistogramDB_< 0 + 0>(db);
				case  0 + 1:  return getHistogramDB_< 0 + 1>(db);

				case 10 + 0:  return getHistogramDB_<10 + 0>(db);
				case 10 + 1:  return getHistogramDB_<10 + 1>(db);

				case 20    :  return getHistogramDB_<20    >(db);
				default:      return nullptr;
				}
			}else{
				switch(id){
				case      0:  return getHistogramDB_<     0>(db);
				default:      return nullptr;
				}
			}
		}

		auto getHistogramCMD(uint64_t id, OutputBlob &blob) -> my_histogram_timer::Timer *{
			switch(id){
			case 0:  return & blob.getTimerCmd();
			default: return nullptr;
			}
		}

		template<class DBAdapter, typename F>
		void forEachDB(DBAdapter const &db, F functor){
			std::string_view const sys = "DB";

			auto f = [&](uint64_t id){
				if (auto *h = getHistogramDB(id, db); h)
					functor(*h, sys, id);
			};



			if constexpr(!DBAdapter::MUTABLE){
				f( 0    );
			}else{
				f( 0 + 0);
				f( 0 + 1);

				f(10 + 0);
				f(10 + 1);

				f(20    );
			}
		}

		template<typename F>
		void forEachCMD(OutputBlob &blob, F functor){
			std::string_view const sys = "CMD";

			auto f = [&](uint64_t id){
				if (auto *h = getHistogramCMD(id, blob); h)
					functor(*h, sys, id);
			};

			f(0);
		}

		template<class DBAdapter, typename F>
		void forEach(DBAdapter const &db, OutputBlob &blob, F functor){
			// proceed for subsystem DB
			forEachDB(db, functor);

			// proceed for subsystem Cmd
			forEachCMD(blob, functor);
		}

		template<bool MUTABLE>
		constexpr std::string_view help(){
			if constexpr(!MUTABLE){
				return
					"DB  :  0 : DISKLIST : R :	\n"
					"\n"
					"CMD :  0 : ALL_CMD  :   :	\n"
				;
			}else{
				return
					"DB  :  0 : ALL      : R :	\n"
					"DB  :  1 : ALL      : W :	\n"
					"\n"
					"DB  : 10 : MEMLIST  : R :	\n"
					"DB  : 11 : MEMLIST  : W :	\n"
					"\n"
					"DB  : 20 : DISKLIST : R :	\n"
					"\n"
					"CMD :  0 : ALL_CMD  :   :	\n"
				;
			}
		}

		template<size_t N>
		std::string_view formatDouble(double d, std::array<char, N> &buffer){
			constexpr static auto fmt_mask = FMT_COMPILE("{:+015.15f}");

			auto const result = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask, d);

			if (result.out == std::end(buffer))
				return {};
			else
				return { buffer.data(), result.size };
		}

	} // namespace latency_impl_



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct LATENCYCOUNTERS : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		LATENCYCOUNTERS() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("LATENCYCOUNTERS", std::begin(cmd__), std::end(cmd__)){}

		// LATENCYCOUNTERS subsystem object_id
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const subsystem = p[1][0];

			using namespace latency_impl_;

			switch(subsystem){
			case SUB_DB[0]:
			case SUB_DB[1]:
				{
					auto const  id   = from_string<uint64_t>(p[2]);
					const auto *hist = getHistogramDB(id, db);

					return process__(id, hist, result, blob);
				}

			case SUB_CMD[0]:
			case SUB_CMD[1]:
				{
					auto const  id   = from_string<uint64_t>(p[2]);
					const auto  hist = getHistogramCMD(id, blob);

					return process__(id, hist, result, blob);
				}

			default:
				// no subsystem selected
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		static void process__(uint64_t id, const my_histogram_timer::Timer *hist, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (!hist)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			else
				return process__(id, *hist, result, blob);
		}

		static void process__(uint64_t , const my_histogram_timer::Timer &hist, Result<Protocol,MemoryManager> &result, OutputBlob &blob){

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



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct LATENCYCOUNT : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		LATENCYCOUNT() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("LATENCYCOUNT", std::begin(cmd__), std::end(cmd__)){}

		// LATENCYCOUNT subsystem object_id
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const subsystem = p[1][0];

			using namespace latency_impl_;

			switch(subsystem){
			case SUB_DB[0]:
			case SUB_DB[1]:
				{
					auto const  id   = from_string<uint64_t>(p[2]);
					const auto *hist = getHistogramDB(id, db);

					return process__(id, hist, result);
				}

			case SUB_CMD[0]:
			case SUB_CMD[1]:
				{
					auto const  id   = from_string<uint64_t>(p[2]);
					const auto  hist = getHistogramCMD(id, blob);

					return process__(id, hist, result);
				}

			default:
				// no subsystem selected
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		static void process__(uint64_t id, const my_histogram_timer::Timer *hist, Result<Protocol,MemoryManager> &result){
			if (!hist)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			else
				return process__(id, *hist, result);
		}

		static void process__(uint64_t, const my_histogram_timer::Timer &hist, Result<Protocol,MemoryManager> &result){
			uint64_t const sum = hist.getCount();

			return result.set(sum);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"latencycount",	"LATENCYCOUNT"
		};
	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct LATENCYPERCENTILE : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		LATENCYPERCENTILE() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("LATENCYPERCENTILE", std::begin(cmd__), std::end(cmd__)){}

		// LATENCYPERCENTILE subsystem object_id percentile
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const subsystem = p[1][0];

			using namespace latency_impl_;

			switch(subsystem){
			case SUB_DB[0]:
			case SUB_DB[1]:
				{
					auto const  id   = from_string<uint64_t>(p[2]);
					const auto *hist = getHistogramDB(id, db);

					auto const  pp   = std::clamp<uint64_t>(from_string<uint64_t>(p[3]), 0, 100);

					return process__(id, pp, hist, result);
				}

			case SUB_CMD[0]:
			case SUB_CMD[1]:
				{
					auto const  id   = from_string<uint64_t>(p[2]);
					const auto  hist = getHistogramCMD(id, blob);

					auto const  pp   = std::clamp<uint64_t>(from_string<uint64_t>(p[3]), 0, 100);

					return process__(id, pp, hist, result);
				}

			default:
				// no subsystem selected
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		static void process__(uint64_t id, uint64_t percentile, const my_histogram_timer::Timer *hist, Result<Protocol,MemoryManager> &result){
			if (!hist)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			else
				return process__(id, percentile, *hist, result);
		}

		static void process__(uint64_t, uint64_t percentile, const my_histogram_timer::Timer &hist, Result<Protocol,MemoryManager> &result){
			using namespace latency_impl_;

			auto const pp = hist.getPercentileUs(static_cast<double>(percentile));

			to_string_buffer_t buffer;

			return result.set(formatDouble(pp, buffer));
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"latencypercentile",	"LATENCYPERCENTILE"
		};
	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct LATENCYMPERCENTILE : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		LATENCYMPERCENTILE() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("LATENCYMPERCENTILE", std::begin(cmd__), std::end(cmd__)){}

		// LATENCYMPERCENTILE subsystem object_id percentile percentile ...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (p.size() < 4)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_3);

			auto const varg  = 3;

			auto const subsystem = p[1][0];

			using namespace latency_impl_;

			switch(subsystem){
			case SUB_DB[0]:
			case SUB_DB[1]:
				{
					auto const  id   = from_string<uint64_t>(p[2]);
					const auto *hist = getHistogramDB(id, db);

					return process__(id, std::begin(p) + varg, std::end(p), hist, result, blob);
				}

			case SUB_CMD[0]:
			case SUB_CMD[1]:
				{
					auto const  id   = from_string<uint64_t>(p[2]);
					const auto  hist = getHistogramCMD(id, blob);

					return process__(id, std::begin(p) + varg, std::end(p), hist, result, blob);
				}

			default:
				// no subsystem selected
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		using IT = ParamContainer::iterator;

		static void process__(uint64_t id, IT first, IT last, const my_histogram_timer::Timer *hist, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (!hist)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			else
				return process__(id, first, last, *hist, result, blob);
		}

		static void process__(uint64_t, IT first, IT last, const my_histogram_timer::Timer &hist, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			using namespace latency_impl_;

			auto &bcontainer	= blob.construct<OutputBlob::SmallBufferContainer>();
			auto &container		= blob.construct<OutputBlob::SmallContainer>();

			for(auto it = first; it != last; ++it){
				auto const percentile	= std::clamp<uint64_t>(from_string<uint64_t>(*it), 0, 100);
				auto const pp		= hist.getPercentileUs(static_cast<double>(percentile));

				bcontainer.push_back();
				container.push_back(formatDouble(pp, bcontainer.back()));
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"latencympercentile",	"LATENCYMPERCENTILE"
		};
	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct LATENCYHUMAN : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		LATENCYHUMAN() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("LATENCYHUMAN", std::begin(cmd__), std::end(cmd__)){}

		// LATENCYHUMAN subsystem object_id
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const subsystem = p[1][0];

			using namespace latency_impl_;

			switch(subsystem){
			case SUB_DB[0]:
			case SUB_DB[1]:
				{
					auto const  id   = from_string<uint64_t>(p[2]);
					const auto *hist = getHistogramDB(id, db);

					return process__(id, hist, result, blob);
				}

			case SUB_CMD[0]:
			case SUB_CMD[1]:
				{
					auto const  id   = from_string<uint64_t>(p[2]);
					const auto hist  = getHistogramCMD(id, blob);

					return process__(id, hist, result, blob);
				}

			default:
				// no subsystem selected
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}
		}

		static void process__(uint64_t id, const my_histogram_timer::Timer *hist, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (!hist)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			else
				return process__(id, *hist, result, blob);
		}

		static void process__(uint64_t, const my_histogram_timer::Timer &hist, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
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



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct LATENCYLOG : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		LATENCYLOG() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("LATENCYLOG", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			using namespace latency_impl_;

			if (p.size() != 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_0);

			auto f = [](auto const &hist, std::string_view sys, uint64_t id){
				logHistogram__(hist, sys, id);
			};

			logger<LOG_LEVEL>() << "Dump all latency histograms:";

			forEach(db, blob, f);

			return result.set();
		}

		static void logHistogram__(my_histogram_timer::Timer const &h, std::string_view sys, uint64_t const id){
			auto const result = h.get(BAR_MAX_WIDTH);

			logger<LOG_LEVEL>() << "System" << sys << "ID" << id << result.total << "events";
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



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct LATENCYRESET : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		LATENCYRESET() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("LATENCYRESET", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (p.size() != 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_0);

			using namespace latency_impl_;

			auto f = [](auto &hist, std::string_view, uint64_t){
				hist.clear();
			};

			forEach(db, blob, f);

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"latencyreset",	"LATENCYRESET"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct LATENCYLIST : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		LATENCYLIST() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("LATENCYLIST", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return result.set(msg__);
		}

	private:
		constexpr static auto msg__ = latency_impl_::help<DBAdapter::MUTABLE>();

	private:
		constexpr inline static std::string_view cmd__[] = {
			"latencylist",	"LATENCYLIST"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "latency";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
				LATENCYCOUNTERS		,
				LATENCYCOUNT		,
				LATENCYPERCENTILE	,
				LATENCYMPERCENTILE	,
				LATENCYHUMAN		,
				LATENCYLOG		,
				LATENCYRESET		,
				LATENCYLIST
			>(pack);
		}
	};



} // namespace



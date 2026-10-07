#include "base.h"
#include "timerlist.h"



namespace net::worker::commands::Latency{

	namespace latency_impl_{
		template<uint64_t id, class DBAdapter>
		auto *getHistogramDB_(bool rw, DBAdapter &db){
			using getH = hm4::chain::get_histogram<id>;

			auto const hrw = rw ? getH::WRITE : getH::READ;

			auto *hist = db->chain(getH{ hrw });

			if constexpr(!std::is_same_v<decltype(hist), const void *>){
				return hist;
			}else{
				// dirty compile test trick :(
				return nullptr;
			}
		}

		template<bool MUTABLE, class DBAdapter>
		auto getHistogramDB(uint64_t id, bool rw, DBAdapter &db) -> my_histogram_timer::Timer *{
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

	} // namespace latency_impl_



	template<class Protocol, class DBAdapter>
	struct LATENCYCOUNTERS : BaseCommandRO<Protocol,DBAdapter>{

		LATENCYCOUNTERS() : BaseCommandRO<Protocol,DBAdapter>("LATENCYCOUNTERS", std::begin(cmd__), std::end(cmd__)){}

		// LATENCYCOUNTERS object_id R/W
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			if constexpr(!DBAdapter::MUTABLE){
				// call 0, read

				auto const id = 0;
				auto const rw = false;

				return process__(id, rw, db, result, blob);
			}else{
				auto const id  = std::clamp<uint64_t>(from_string<uint64_t>(p[1]), 0, 2);
				auto const rw  = p[2] == "w" || p[2] == "W";

				return process__(id, rw, db, result, blob);
			}
		}

		static void process__(uint64_t id, bool rw, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace latency_impl_;

			const auto *hist = getHistogramDB<DBAdapter::MUTABLE>(id, rw, db);

			if (!hist)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto &container  = blob.construct<OutputBlob::SmallContainer>();
			auto &bcontainer = blob.construct<OutputBlob::SmallBufferContainer>();

			for(auto const &num : *hist){
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
	struct LATENCYHUMAN : BaseCommandRO<Protocol,DBAdapter>{

		LATENCYHUMAN() : BaseCommandRO<Protocol,DBAdapter>("LATENCYHUMAN", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			if constexpr(!DBAdapter::MUTABLE){
				// call 0, read

				auto const id = 0;
				auto const rw = false;

				return process__(id, rw, db, result, blob);
			}else{
				auto const id  = std::clamp<uint64_t>(from_string<uint64_t>(p[1]), 0, 2);
				auto const rw  = p[2] == "w" || p[2] == "W";

				return process__(id, rw, db, result, blob);
			}
		}

		static void process__(uint64_t id, bool rw, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace latency_impl_;

			const auto *hist = getHistogramDB<DBAdapter::MUTABLE>(id, rw, db);

			if (!hist)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto &buffer  = blob.allocate<std::array<char, BUFFER_SIZE> >();

			      auto *ptr = buffer.data();
			const auto *end = buffer.data() + buffer.size();

			auto const hresult = hist->get(BAR_MAX_WIDTH);

			for(size_t i = 0; i < hist->size(); ++i){
				auto const &row = hresult.events[i];

			//	if (!row.value)
			//		continue;

				size_t const remaining = static_cast<size_t>(end - ptr);

				auto const res = fmt::format_to_n(
					ptr,
					remaining,
					FMT_COMPILE("[ {} - {} ) {:10} {:8.2f} {:8.2f} {:.{}}{:.{}}\n"),

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

		constexpr static size_t			BAR_MAX_WIDTH	= 14;
		constexpr static std::string_view	BAR_FULL	= "#####" "#####" "#####" "#####";
	//	constexpr static std::string_view	BAR_FULL	= "▪▪▪▪▪" "▪▪▪▪▪" "▪▪▪▪▪" "▪▪▪▪▪";
		constexpr static std::string_view	BAR_EMPTY	= "     " "     " "     " "     ";

	private:
		constexpr inline static std::string_view cmd__[] = {
			"latencyhuman",	"LATENCYHUMAN"
		};

	};



	template<class Protocol, class DBAdapter>
	struct LATENCYLOG : BaseCommandRO<Protocol,DBAdapter>{

		LATENCYLOG() : BaseCommandRO<Protocol,DBAdapter>("LATENCYLOG", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			auto L = [&db](uint64_t id, bool rw){
				using namespace latency_impl_;

				const auto *hist = getHistogramDB<DBAdapter::MUTABLE>(id, rw, db);

				if (hist){
					std::string_view const msg = rw ? "Write" : "Read";

					logHistogram__(*hist, id, msg);
				}
			};

			if constexpr(!DBAdapter::MUTABLE){
				// call 0, read

				auto const id = 0;

				L(id, 0);

				result.set();
			}else{
				auto const id  = std::clamp<uint64_t>(from_string<uint64_t>(p[1]), 0, 2);

				L(id, 0);
				L(id, 1);

				result.set();
			}
		}

		static void logHistogram__(my_histogram_timer::Timer const &h, uint64_t const id, std::string_view msg){
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

	private:
		constexpr static size_t			BUFFER_SIZE	= 2048; // 1910 when ASCII;

	private:
		constexpr static size_t			BAR_MAX_WIDTH	= 14;
		constexpr static std::string_view	BAR_FULL	= "#####" "#####" "#####" "#####";
	//	constexpr static std::string_view	BAR_FULL	= "▪▪▪▪▪" "▪▪▪▪▪" "▪▪▪▪▪" "▪▪▪▪▪";
		constexpr static std::string_view	BAR_EMPTY	= "     " "     " "     " "     ";

		constexpr static Logger::Level		LOG_LEVEL	= Logger::DEBUG;

	private:
		constexpr inline static std::string_view cmd__[] = {
			"latencylog",	"LATENCYLOG"
		};

	};



	template<class Protocol, class DBAdapter>
	struct LATENCYRESET : BaseCommandRO<Protocol,DBAdapter>{

		LATENCYRESET() : BaseCommandRO<Protocol,DBAdapter>("LATENCYRESET", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_0);

			auto R = [&db](uint64_t id, bool rw){
				using namespace latency_impl_;

				auto *hist = getHistogramDB<DBAdapter::MUTABLE>(id, rw, db);

				if ( hist)
					hist->clear();
			};

			if constexpr(!DBAdapter::MUTABLE){
				R(0, 0);

				result.set();
			}else{
				R(0, 0);
				R(1, 0);
				R(2, 0);

				R(0, 1);
				R(1, 1);
				R(2, 1);

				result.set();
			}
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
				LATENCYHUMAN	,
				LATENCYLOG	,
				LATENCYRESET
			>(pack);
		}
	};



} // namespace



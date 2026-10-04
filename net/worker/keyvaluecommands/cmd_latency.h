#include "base.h"
#include "timerlist.h"	// logHistogram



namespace net::worker::commands::Latency{



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

				switch(id){
				default:
				case 0: return process__<0>(rw, db, result, blob);
				}
			}else{
				auto const id  = std::clamp<uint64_t>(from_string<uint64_t>(p[1]), 0, 2);
				auto const rw  = p[2] == "w" || p[2] == "W";

				switch(id){
				default:
				case 0: return process__<0>(rw, db, result, blob);
				case 1: return process__<1>(rw, db, result, blob);
				case 2: return process__<2>(rw, db, result, blob);
				}
			}
		}

		template<uint64_t ID>
		static void process__(bool rw, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using get_histogram = hm4::chain::get_histogram<ID>;

			auto const hrw = rw ? get_histogram::WRITE : get_histogram::READ;

			const auto *hist = db->chain(get_histogram{ hrw });

			if (!hist)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			if constexpr(!std::is_same_v<decltype(hist), const void *>){
				auto &container  = blob.construct<OutputBlob::SmallContainer>();
				auto &bcontainer = blob.construct<OutputBlob::SmallBufferContainer>();

				for(auto const &num : *hist){
					bcontainer.push_back();

					container.push_back(to_string(num, bcontainer.back()));
				}

				return result.set_container(container);
			}else{
				return result.set_containerN();
			}
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

				switch(id){
				default:
				case 0: return process__<0>(rw, db, result, blob);
				}
			}else{
				auto const id  = std::clamp<uint64_t>(from_string<uint64_t>(p[1]), 0, 2);
				auto const rw  = p[2] == "w" || p[2] == "W";

				switch(id){
				default:
				case 0: return process__<0>(rw, db, result, blob);
				case 1: return process__<1>(rw, db, result, blob);
				case 2: return process__<2>(rw, db, result, blob);
				}
			}
		}

		template<uint64_t ID>
		static void process__(bool rw, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using get_histogram = hm4::chain::get_histogram<ID>;

			auto const hrw = rw ? get_histogram::WRITE : get_histogram::READ;

			const auto *hist = db->chain(get_histogram{ hrw });

			if (!hist)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			if constexpr(!std::is_same_v<decltype(hist), const void *>){
				auto &buffer  = blob.construct<std::array<char, BUFFER_SIZE> >();

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
			}else{
				return result.set();
			}
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

			if constexpr(!DBAdapter::MUTABLE){
				// call 0, read

				auto const id = 0;

				switch(id){
				default:
				case 0: return process__<0>(db, result);
				}
			}else{
				auto const id  = std::clamp<uint64_t>(from_string<uint64_t>(p[1]), 0, 2);

				switch(id){
				default:
				case 0: return process__<0>(db, result);
				case 1: return process__<1>(db, result);
				case 2: return process__<2>(db, result);
				}
			}
		}

		template<uint64_t ID>
		static void process__(DBAdapter &db, Result<Protocol> &result){
			db->chain(hm4::chain::log_histogram<ID>{});
			result.set();
       		}

	private:
		constexpr static size_t			BUFFER_SIZE	= 2048; // 1910 when ASCII;

		constexpr static std::string_view	ERROR_MESSAGE	= "INTERNAL ERROR IN LATENCYHUMAN, PLEASE REPORT A BUG!";

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

		void process(ParamContainer const &, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if constexpr(!DBAdapter::MUTABLE){
				db->chain(hm4::chain::reset_histogram<0>{});	// main counter

				return result.set();
			}else{
				db->chain(hm4::chain::reset_histogram<0>{});	// main counter
				db->chain(hm4::chain::reset_histogram<1>{});	// mutable counter
				db->chain(hm4::chain::reset_histogram<2>{});	// immutable counter

				return result.set();
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



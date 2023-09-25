#define FMT_HEADER_ONLY
#include "fmt/printf.h"

#include "version.h"

#include "myglob.h"
#include "mytime.h"
#include "disk/disklist.h"

namespace{

	int printUsage(const char *cmd){
		fmt::print(
			"db_compact_tombstones version {version}\n"
			"\n"
			"Build:\n"
			"\tDate       : {date} {time}\n"
			"\n"
			"Usage:\n"
			"\t{cmd} [lsm_path] - analyze LSM and suggest compaction table.\n"
			"\n"
			,

			fmt::arg("version",	hm4::version::str	),
			fmt::arg("date",	__DATE__		),
			fmt::arg("time",	__TIME__		),
			fmt::arg("cmd",		cmd			)
		);

		return 10;
	}

	namespace tombstone_calculator_{

		template<typename T>
		struct Range;

		template<typename T>
		struct EndlessRange{
			T start		= std::numeric_limits<T>::max();

			constexpr auto &expand(Range<T> const range){
				start = std::min(start, range.start);
				return *this;
			}
		};

		template<typename T>
		struct Range{
			T start		= std::numeric_limits<T>::max();
			T finish	= std::numeric_limits<T>::max();

			constexpr int compare(Range const other) const{
				if (finish < other.start){
					// this is less than other
					return -1;
				}

				if (start > other.finish){
					// this is greater than other
					return +1;
				}

				// ranges overlaps
				return 0;
			}

			constexpr int compare(EndlessRange<T> const other) const{
				if (finish < other.start){
					// this is less than other
					return -1;
				}

				// ranges overlaps
				return 0;
			}
		};



		namespace test{
			using T = unsigned short;
			using _ = Range<T>;
			using e = EndlessRange<T>;

			static_assert(_{10, 12}.compare(_{13, 15}) == -1);
			static_assert(_{13, 15}.compare(_{10, 12}) == +1);

			static_assert(_{15, 25}.compare(_{20, 30}) ==  0);
			static_assert(_{20, 30}.compare(_{15, 25}) ==  0);

			static_assert(_{20, 30}.compare(_{10, 40}) ==  0);
			static_assert(_{10, 40}.compare(_{20, 30}) ==  0);

			static_assert(_{10, 12}.compare(e{14}) == -1);
			static_assert(_{13, 15}.compare(e{14}) ==  0);

			static_assert(e{15}.expand(_{20, 12}).start == 15);
			static_assert(e{15}.expand(_{10, 12}).start == 10);

		}



		template<typename T, typename Payload>
		struct TombstoneCalculator{
			constexpr void operator()(T start, T finish, Payload const &payload){
				return insert_(Range{ start, finish }, payload);
			}

			constexpr auto operator()() const{
				return payload_;
			}

		private:
			using Range		= tombstone_calculator_::Range<T>;
			using EndlessRange	= tombstone_calculator_::EndlessRange<T>;

			constexpr void insert_(Range const range, Payload const &payload){
				if (range.compare(forbidden_) == 0){
					// can not really be large
					// overlaps
					forbidden_.expand(range);

					return;
				}

				auto const i = range.compare(current_);

				if (i == 0){
					// overlaps
					// invalide both
					forbidden_.expand(range);
					forbidden_.expand(current_);
					current_ = {};
					payload_ = {};

					return;
				}

				if (i > 0){
					// range is younger, current stay
					// invalidate range
					forbidden_.expand(range);
				}else{
					// range is older and is new current
					// invalidate current
					forbidden_.expand(current_);
					current_ = range;
					payload_ = payload;
				}
			}

		private:
			EndlessRange	forbidden_;
			Range		current_;
			Payload		payload_{};
		};

	} // namespace tombstone_calculator_



	auto getInfo(std::string_view filename){
		using hm4::disk::DiskList;

		constexpr auto DEFAULT_ADVICE	= MMAPFile::Advice::SEQUENTIAL;
		constexpr auto DEFAULT_MODE	= DiskList::OpenMode::FORWARD;

		hm4::disk::DiskList list;
		list.open(filename, DEFAULT_ADVICE, DEFAULT_MODE);

		struct ListInfo{
			uint64_t time_min;
			uint64_t time_max;
		};

		return std::make_pair(
			list.createdMin(),
			list.createdMax()
		);
	}

	void analyze(const char *db_path){
		auto _ = [](auto date, auto &buffer){
			constexpr auto time_format = mytime::TIME_FORMAT_STANDARD;

			return date    ? mytime::toString(time_format, buffer) : "n/a";
		};

		MyGlob gl;
		gl.open(db_path);

		mytime::to_string_buffer_t buffer[2];

		tombstone_calculator_::TombstoneCalculator<uint64_t, const char *> tc;

		for(auto &filename : gl){
			auto const [time_min, time_max] = getInfo(filename);

			fmt::print("{:<62} | {:10} | {:10}\n", filename, _(time_min, buffer[0]), _(time_max, buffer[1]));

			tc(time_min, time_max, filename);
		}

		std::string_view x = tc();

		constexpr std::string_view mask = "\nResult:\n" "\t{}\n";

		if (x.empty()){
			fmt::print(mask, "Can not compact anything!");
		}else{
			fmt::print(mask, x);
		}
	}

}

int main(int argc, char **argv){
	if (argc != 2)
		return printUsage(argv[0]);

	getLoggerSingleton().setLevel(0);

	const char *db_path	= argv[1];

	analyze(db_path);
}


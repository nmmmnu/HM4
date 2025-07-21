#ifndef SHARED_RANDOM_H_
#define SHARED_RANDOM_H_

#include <random>
#include <array>
#include <algorithm>	// generate

namespace net::worker::shared::random{
	namespace {

		auto factory_(){
			using T = uint32_t;

			static_assert(std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>, "RandomGenerator only supports uint32_t or uint64_t");

			using MT = typename std::conditional_t<
					std::is_same<T, uint32_t>::value,
					std::mt19937,
					std::mt19937_64
			>;

			MT gen{ 0 };

			std::array<T, 4096> data;

			std::generate(std::begin(data), std::end(data), [&](){
				return gen();
			});

			return data;
		}

	} // anonymous namespace

	auto &get(){
		static std::array<uint32_t, 4096> const data = factory_();

		return data;
	}

	auto get(size_t index){
		auto &data = get();

		return data[index % data.size()];
	}
}

#endif


#ifndef SHARED_RANDOM_H_
#define SHARED_RANDOM_H_

#include <random>
#include <array>
#include <algorithm>	// generate

namespace net::worker::shared::random{

	template<size_t Size>
	struct RandomContainer{
		RandomContainer(uint64_t seed = 0){
			std::mt19937_64 gen{ seed };

			using T = uint64_t;

			std::generate(data<T>(), data<T>() + size<T>(), [&](){
				return gen();
			});
		}

		template<typename T>
		constexpr static size_t size(){
			if constexpr(std::is_same_v<T, uint64_t>) return Size * 1;
			if constexpr(std::is_same_v<T, uint32_t>) return Size * 2;
			if constexpr(std::is_same_v<T, uint16_t>) return Size * 4;
			if constexpr(std::is_same_v<T, uint8_t >) return Size * 8;
		}

		template<typename T>
		constexpr auto *data(){
			if constexpr(std::is_same_v<T, uint64_t>) return data8_;
			if constexpr(std::is_same_v<T, uint32_t>) return data4_;
			if constexpr(std::is_same_v<T, uint16_t>) return data2_;
			if constexpr(std::is_same_v<T, uint8_t >) return data1_;
		}

		template<typename T>
		constexpr auto const *data() const{
			if constexpr(std::is_same_v<T, uint64_t>) return data8_;
			if constexpr(std::is_same_v<T, uint32_t>) return data4_;
			if constexpr(std::is_same_v<T, uint16_t>) return data2_;
			if constexpr(std::is_same_v<T, uint8_t >) return data1_;
		}

	private:
		union{
			uint64_t data8_[size<uint64_t>()];
			uint32_t data4_[size<uint32_t>()];
			uint16_t data2_[size<uint16_t>()];
			uint8_t  data1_[size<uint8_t >()];
		};
	};

	template<typename T>
	auto get(size_t index){
		constexpr size_t RandomContainerSize = 4096;

		static RandomContainer<RandomContainerSize> const rc;

		auto const &data = rc.data<T>();

		return data[index % rc.size<T>()];
	}
}

#endif


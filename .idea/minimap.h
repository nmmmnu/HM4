#ifndef MY_MINI_MAP_H_
#define MY_MINI_MAP_H_

#include <vector>
#include <string_view>
#include <utility>

#include "binarysearch.h"

namespace mini_map{

	template<typename K, typename V>
	using Pair   = std::pair<K, V>;

	template<typename K, typename V>
	using Vector = std::vector<Pair<K, V> >;

	template<typename K, typename V>
	void done(Vector<K, V> &data){
		auto gcomp = [](const auto &a, const auto &b){
			return a.first < b.first;
		};

		auto ecomp = [](const auto &a, const auto &b){
			return a.first == b.first;
		};

		std::sort(std::begin(data), std::end(data), gcomp);

		data.erase(
			std::unique( std::begin(data), std::end(data), ecomp),
			std::end(data)
		);
	}

	namespace mini_map_impl_{
		auto comp_str = [](const auto &p, std::string_view const key){
			return compare(p.first.data(), p.first.size(), key.data(), key.size());
		};
	}

	template<typename V>
	auto find(Vector<std::string_view, V> const &data, std::string_view key){
		using namespace mini_map_impl_;
		return binarySearch(std::begin(data), std::end(data), key, comp_str);
	}

	template<typename V>
	auto find(Vector<std::string, V> const &data, std::string_view key){
		using namespace mini_map_impl_;
		return binarySearch(std::begin(data), std::end(data), key, comp_str);
	}

}

#endif


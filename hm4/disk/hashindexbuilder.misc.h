#ifndef HASH_INDEX_BUILDER_ALGO_MISC_H_
#define HASH_INDEX_BUILDER_ALGO_MISC_H_

#include "myendian.h"
#include "mybuffer.h"
#include "bufferedvector.h"

#include "pair.h"

#include <cstdio> // printf

namespace hm4::disk::hash::algo{

	template<typename T>
	struct NodeHelper{
		uint64_t	hash	= 0;
		T		pos	= 0;

		static_assert(
			std::is_same_v<T, uint32_t> ||
			std::is_same_v<T, uint64_t>
		);

		constexpr NodeHelper() = default;
		constexpr NodeHelper(uint64_t hash, T pos): hash(hash), pos(pos){}

		constexpr size_t cell(size_t count) const{
			return hash % count;
		}

		constexpr Node getNodeBE() const{
			return Node{
				htobe<uint64_t>(hash	),
				htobe<uint64_t>(pos	)
			};
		}
	}
	__attribute__((__packed__));

	static_assert(std::is_standard_layout<NodeHelper<uint64_t> >::value, "NodeHelper32 must be POD type");
	static_assert(std::is_standard_layout<NodeHelper<uint32_t> >::value, "NodeHelper64 must be POD type");

	template<typename T>
	using NodeHelperBuffer = MyBuffer::BufferView<NodeHelper<T> >;

	template<typename T>
	using NodeHelperVector = BufferedVector<NodeHelper<T>, NodeHelperBuffer<T> >;



	template<typename T, typename NodeContainer, typename Projection>
	bool insert__(NodeHelper<T> const &data, NodeContainer &nodes, size_t const nodesCount, Projection f){
		auto const hc = data.hash;

		for(size_t i = 0; i < HASHTABLE_OPEN_ADDRESSES; ++i){
			// branchless
			auto const cell = (hc + i) % nodesCount;

			if (auto &node = nodes[cell]; !node.hash){
				node = f(data);

				return true;
			}
		}

		return false;
	}


} // namespace hm4::disk::hash::algo

#endif


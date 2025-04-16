#ifndef HASH_INDEX_NODE_H
#define HASH_INDEX_NODE_H_

#include "hashindexnode.h"
#include "mmapfile.h"
#include "mybufferview.h"
#include "bufferedvector.h"

#include "disk/disklist.h"

#include <limits>

namespace hm4::disk::hash{



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



	struct HashIndexBuilder{
		constexpr static uint8_t EXPAND_FACTOR			= 33; // 1.33 %

		HashIndexBuilder(std::string_view filename, size_t listSize) :
								filename_	(filename	),
								nodesCount_	(listSize + listSize * EXPAND_FACTOR / 100	){}

		bool process(DiskList const &list, MyBuffer::ByteBufferView buffer){
			auto print = [](const char *s, auto t){
				using T = decltype(t);

				size_t const u = sizeof(T) * 8;

				printf("\n");
				printf("Using %s %zu algorithm\n", s, u);
				printf("\n");
			};

			if (list.size() <= std::numeric_limits<uint32_t>::max()){
				using T = uint32_t;

				if (buffer.size() >= nodesCount_ * sizeof(NodeHelper<T>)){
					print("standard",   T{});
					return processStandard_<T>(list, buffer);
				}else{
					print("multi-pass", T{});
					return processMultiPass_<T>(list, buffer);
				}
			}else{
				using T = uint64_t;

				if (buffer.size() >= nodesCount_ * sizeof(NodeHelper<T>)){
					print("standard",   T{});
					return processStandard_<T>(list, buffer);
				}else{
					print("multi-pass", T{});
					return processMultiPass_<T>(list, buffer);
				}
			}
		}

	private:
		enum class ALGO{
			STANDARD32	,
			STANDARD64	,
			MULTIPASS32	,
			MULTIPASS64
		};

	private:
		template<typename T>
		bool processMultiPass_(DiskList const &list, MyBuffer::ByteBufferView buffer);

		template<typename T>
		bool processStandard_(DiskList const &list, MyBuffer::ByteBufferView buffer);

	private:
		std::string_view	filename_;

		size_t			nodesCount_;

		ALGO			algo_;
	};



} // namespace hm4::disk::hash

#include "hashindexbuilder.cc.h"

#endif


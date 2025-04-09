#ifndef HASH_INDEX_BUILDER_H
#define HASH_INDEX_BUILDER_H_

#include "hashindexnode.h"

#include "hashindexbuilder.multipass.h"
#include "hashindexbuilder.standard.h"

#include "oprime.h"

#include "logger.h"

#include <variant>
#include <limits>

namespace hm4::disk::hash{

	struct HashIndexBuilder{
		constexpr static double EXPAND_FACTOR = 1.333333;

		HashIndexBuilder(std::string_view filename, size_t listSize, MyBuffer::ByteBufferView buffer) :
								impl_( selectImplementation__(filename, listSize, buffer) ){}

		void operator()(Pair const &pair){
			auto visitor = [&pair](auto &x){
				x(pair);
			};

			std::visit(visitor, impl_);
		}

	private:
		using Implementation = std::variant<
			algo::HashIndexStandardBuilder<uint32_t>,
			algo::HashIndexStandardBuilder<uint64_t>,

			algo::HashIndexMultiPassBuilder<uint32_t>,
			algo::HashIndexMultiPassBuilder<uint64_t>
		>;

	private:
		constexpr static size_t selectNodeCount__(size_t const listSize){
			return oprime::next(
				static_cast<size_t>(
					static_cast<double>(listSize) * EXPAND_FACTOR
				)
			);
		}

		template<typename T>
		static Implementation selectImplementation__2(std::string_view filename, size_t nodesCount, MyBuffer::ByteBufferView buffer){
			auto print = [nodesCount](const char *s){
				size_t const u = sizeof(T) * 8;

				logger<Logger::NOTICE>() << "Using" << s << u << "bit algorithm, with" << nodesCount << "nodes.";
			};

			if (buffer.size() >= nodesCount * sizeof(algo::NodeHelper<T>)){
				print("standard");
				return Implementation{ std::in_place_type<algo::HashIndexStandardBuilder<T> >, filename, nodesCount, buffer };
			}else{
				print("multi-pass");
				return Implementation{ std::in_place_type<algo::HashIndexMultiPassBuilder<T> >, filename, nodesCount, buffer };
			}

		}

		static Implementation selectImplementation__(std::string_view filename, size_t listSize, MyBuffer::ByteBufferView buffer){
			size_t const nodesCount = selectNodeCount__(listSize);

			if (nodesCount <= std::numeric_limits<uint32_t>::max())
				return selectImplementation__2<uint32_t>(filename, nodesCount, buffer);
			else
				return selectImplementation__2<uint64_t>(filename, nodesCount, buffer);
		}

	private:
		Implementation	impl_;
	};

} // namespace hm4::disk::hash

#endif



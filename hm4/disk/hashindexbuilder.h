#ifndef HASH_INDEX_BUILDER_H_
#define HASH_INDEX_BUILDER_H_

#include "disk/filenames.h"

#include "hashindexnode.h"

#include "hashindexbuilder.multipass.h"
#include "hashindexbuilder.standard.h"

#include "oprime.h"

#include "logger.h"

#include "mybufferview.h"
#include "mmapbuffer.h"

#include <variant>
#include <limits>
#include <cassert>

namespace hm4::disk::hash{

	struct HashIndexBuilder{
		constexpr static double EXPAND_FACTOR = 1.333333;

		HashIndexBuilder(std::string_view filename, MyBuffer::ByteBufferView bufferWrite, size_t listSize, MyBuffer::ByteBufferView buffer) :
								filename_	(filenameHash(filename)	),
								guard_		(buffer			),
								impl_		( selectImplementation__(filename_, bufferWrite, listSize, buffer)	){}

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
		static Implementation selectImplementation__2(std::string_view filename, MyBuffer::ByteBufferView bufferWrite, size_t nodesCount, MyBuffer::ByteBufferView buffer){
			auto print = [nodesCount](const char *s){
				size_t const u = sizeof(T) * 8;

				logger<Logger::NOTICE>() << "Using" << s << u << "bit HashIndex builder, with" << nodesCount << "nodes.";
			};

			if (buffer.size() >= nodesCount * sizeof(algo::NodeHelper<T>)){
				print("standard");
				return Implementation{ std::in_place_type<algo::HashIndexStandardBuilder<T> >, filename, bufferWrite, nodesCount, buffer };
			}else{
				print("multi-pass");
				return Implementation{ std::in_place_type<algo::HashIndexMultiPassBuilder<T> >, filename,             nodesCount, buffer };
			}

		}

		static Implementation selectImplementation__(std::string_view filename, MyBuffer::ByteBufferView bufferWrite, size_t listSize, MyBuffer::ByteBufferView buffer){
			assert(listSize > 0);

			size_t const nodesCount = selectNodeCount__(listSize);

			if (nodesCount <= std::numeric_limits<uint32_t>::max())
				return selectImplementation__2<uint32_t>(filename, bufferWrite, nodesCount, buffer);
			else
				return selectImplementation__2<uint64_t>(filename, bufferWrite, nodesCount, buffer);
		}

	private:
		std::string			filename_;
		MyBuffer::MMapAdviceGuard	guard_;
		Implementation			impl_;
	};

} // namespace hm4::disk::hash

#endif



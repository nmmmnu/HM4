#include "hashindexbuilder.misc.h"

#include "filewriter.h"

#include <cstring>	// memset

namespace hm4::disk::hash::algo{

	template<typename T>
	bool insertHH(NodeHelper<T> const &data, NodeHelperBuffer<T> &nodes, size_t const nodesCount){
		auto f = [](auto const &data){
			return data;
		};

		return insert__(data, nodes, nodesCount, f);
	}



	template<typename T>
	struct HashIndexStandardBuilder{
		HashIndexStandardBuilder(std::string_view filename, MyBuffer::ByteBufferView bufferWrite, size_t nodesCount, MyBuffer::ByteBufferView buffer) :
								buffer_		(buffer		),
								nodesCount_	(nodesCount	),
								filename_	(filename	),
								bufferWrite_	(bufferWrite	){

			memset( static_cast<void *>(buffer_.data()), 0, nodesCount_ * sizeof(NodeHelper<T>));
		}

		bool operator()(Pair const &pair){
			auto const data = NodeHelper<T>{ murmur_hash64a(pair.getKey()), pos_++ };

			return insertHH(data, buffer_, nodesCount_);
		}

		~HashIndexStandardBuilder(){
			FileWriter file{ filename_, bufferWrite_ };

			logger<Logger::NOTICE>() << "HashIndex save index...";

			for(size_t i = 0; i < nodesCount_; ++i){
				auto const node = buffer_[i].getNodeBE();
				file.write(& node, sizeof(Node));
			}

			logger<Logger::NOTICE>() << "HashIndex save index done.";
		}

	private:
		T				pos_		= 0;
		NodeHelperBuffer<T>		buffer_		;
		size_t				nodesCount_	;
		std::string_view 		filename_	;
		MyBuffer::ByteBufferView	bufferWrite_	;
	};

} // namespace hm4::disk::hash::algo


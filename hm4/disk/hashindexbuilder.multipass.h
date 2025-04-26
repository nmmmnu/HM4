#include "hashindexbuilder.misc.h"

#include "mmapfile.h"

#include "logger.h"

#include <algorithm>	// sort

namespace hm4::disk::hash::algo{

	template<typename T>
	MMAPFileRW createMMAP(std::string_view filename, size_t const size){
		MMAPFileRW mmap;

		mmap.create(filename, MMAPFile::Advice::SEQUENTIAL, size * sizeof(T));

		return mmap;
	}



	template<typename T>
	bool insertMM(NodeHelper<T> const &data, Node *nodes, size_t const nodesCount){
		auto f = [](auto const &data){
			return data.getNodeBE();
		};

		return insert__(data, nodes, nodesCount, f);
	}



	template<typename T>
	void flush(NodeHelperVector<T> &vector, Node *nodes, size_t const nodesCount){
		if (!nodes)
			return;

		std::sort(std::begin(vector), std::end(vector), [c = nodesCount](auto const &a, auto const &b){
			// we skipping disk operations here, so it OK
			return a.cell(c) < b.cell(c);
		});

		for(auto const &node : vector)
			insertMM(node, nodes, nodesCount);
	}



	template<typename T>
	struct HashIndexMultiPassBuilder{
		HashIndexMultiPassBuilder(std::string_view filename, size_t nodesCount, MyBuffer::ByteBufferView buffer) :
								vector_		(buffer		),
								nodesCount_	(nodesCount	),
								mmap_		(createMMAP<Node>(filename, nodesCount) ){}

		bool operator()(Pair const &pair){
			if (!mmap_)
				return false;

			vector_.emplace_back(murmur_hash64a(pair.getKey()), pos_++);

			if (vector_.full()){
				logger<Logger::NOTICE>() << "HashIndex buffer flush... Processed" << vector_.size() << "record(s), total record(s)" << pos_;
				flush(vector_, nodes_(), nodesCount_);
				logger<Logger::NOTICE>() << "HashIndex buffer flush done.";

				vector_.clear();
			}

			return true;
		}

		~HashIndexMultiPassBuilder(){
			if (!vector_.empty()){
				logger<Logger::NOTICE>() << "HashIndex buffer flush... Processed" << vector_.size() << "record(s), total record(s)" << pos_;
				flush(vector_, nodes_(), nodesCount_);
				logger<Logger::NOTICE>() << "HashIndex buffer flush done.";
			}
		}

	private:
		Node *nodes_(){
			return static_cast<Node *>(mmap_.data());
		}

	private:
		T			pos_		= 0;
		NodeHelperVector<T>	vector_		;
		size_t			nodesCount_	;
		MMAPFileRW		mmap_		;
	};

} // namespace hm4::disk::hash::algo


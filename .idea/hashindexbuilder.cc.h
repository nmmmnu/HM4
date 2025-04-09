#include <cstdlib>	// malloc

#include "myendian.h"
#include "filewriter.h"

namespace hm4::disk::hash{

namespace hash_impl_{

	Node *createMMAP(MMAPFile &mmap, std::string_view filename, size_t size){
		mmap.create(filename, MMAPFile::Advice::SEQUENTIAL, size);

		if (!mmap)
			return nullptr;

		auto *mem = mmap.memRW();

		if (!mem)
			return nullptr;

		return reinterpret_cast<Node *>(mem);
	}



	template<typename T, typename NodeContainer, typename F>
	bool insert__(NodeHelper<T> const &data, NodeContainer &nodes, size_t const nodesCount, F f){
		auto cell = data.cell(nodesCount);

		for(size_t i = 0; i < HASHTABLE_OPEN_ADDRESSES; ++i){
			if (auto &node = nodes[cell]; !node.hash){
				node = f(data);

				return true;
			}

			// branchless
			cell = (cell + 1) % nodesCount;
		}

		return false;
	}

	template<typename T>
	bool insertMM(NodeHelper<T> const &data, Node *nodes, size_t const nodesCount){
		auto f = [](auto const &data){
			return data.getNodeBE();
		};

		return insert__(data, nodes, nodesCount, f);
	}

	// merging both insert() result in unreadable code

	template<typename T>
	bool insertHH(NodeHelper<T> const &data, NodeHelperBuffer<T> &nodes, size_t const nodesCount){
		auto f = [](auto const &data){
			return data;
		};

		return insert__(data, nodes, nodesCount, f);
	}



	template<typename T>
	void flush(NodeHelperVector<T> &vector, Node *nodes, size_t const nodesCount){
		printf("\tSort.... ");

		std::sort(std::begin(vector), std::end(vector), [c = nodesCount](auto const &a, auto const &b){
			// we skipping disk operations here, so it OK
			return a.cell(c) < b.cell(c);
		});

		printf("done\n");

		printf("\tWrite... ");

		for(auto const &node : vector){
			[[maybe_unused]]
			auto const inserted = insertMM(node, nodes, nodesCount);

			// if (! inserted )
			// 	printf("Skip insert %16lx.\n", node->hash);
		}

		printf("done\n");
	}

} //namespace impl_



template<typename T>
bool HashIndexBuilder::processMultiPass_(DiskList const &list, MyBuffer::ByteBufferView byteBuffer){
	MMAPFile mmap;

	Node *nodes = hash_impl_::createMMAP(mmap, filename_, nodesCount_ * sizeof(Node));

	if ( !nodes){
		printf("Can not create hash index file.\n");
		return false;
	}

	NodeHelperVector<T> vector{ byteBuffer };

	T pos = 0;

	for(auto &p : list){
		vector.emplace_back(murmur_hash64a(p.getKey()), pos);

		if (vector.full()){
			printf("Processed %10zu records, total records %10zu, buffer flush...\n", vector.size(), size_t{pos});

			hash_impl_::flush(vector, nodes, nodesCount_);

			vector.clear();
		}

		++pos;
	}

	if (!vector.empty()){
			printf("Processed %10zu records, total records %10zu, buffer final flush...\n", vector.size(), size_t{pos});

			hash_impl_::flush(vector, nodes, nodesCount_);
	}

	return true;
}



template<typename T>
bool HashIndexBuilder::processStandard_(DiskList const &list, MyBuffer::ByteBufferView byteBuffer){

	memset(byteBuffer.data(), 0, nodesCount_ * sizeof(NodeHelper<T>));

	NodeHelperBuffer<T> buffer = byteBuffer;

	T pos = 0;

	for(auto &p : list){
		auto const data = NodeHelper<T>{
					murmur_hash64a(p.getKey()),
					pos
		};

		hash_impl_::insertHH(data, buffer, nodesCount_);

		++pos;
	}

	FileWriter file{ filename_ };

	// underline data is not in
	for(size_t i = 0; i < nodesCount_; ++i){
		auto const node = buffer[i].getNodeBE();
		file.write(& node, sizeof(Node));
	}

	return true;
}



} // namespace hm4::disk::hash



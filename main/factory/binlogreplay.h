#ifndef BIN_LOG_REPLAY_H_
#define BIN_LOG_REPLAY_H_

namespace DBAdapterFactory{

	template<class MyMemList>
	struct BinLogReplay{
		using MemList		= MyMemList;
		using Predicate		= hm4::flusher::DiskFileAllocatorPredicate;
		using IDGenerator	= idgenerator::IDGeneratorDate;
		using Flush		= hm4::flusher::DiskFileFlush<IDGenerator>;
		using MyList		= hm4::FlushList<MemList,Predicate, Flush>;

		template<typename UString>
		BinLogReplay(UString &&path, typename MemList::Allocator &allocator, MyBuffer::ByteBufferView bufferPair) :
					memlist{ allocator },
					mylist{
						memlist,
						bufferPair,
						{}, // bufferHash
						Predicate{},
						Flush{ IDGenerator{}, std::forward<UString>(path) }
					}{}

		MyList &operator()(){
			return mylist;
		}

	private:
		MemList	memlist;
		MyList	mylist;
	};

}

#endif


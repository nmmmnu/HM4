#include "disklist.h"
#include "hpair.h"

#include "disk/filenames.h"
#include "disk/btreeindexnode.h"

#include "stringhash.h"

#include "hashindexnode.h"

#include "binarysearch.h"

#include "myalign.h"

#include "logger.h"


namespace hm4{
namespace disk{

struct SmallNode{
	HPair::HKey	key;
	uint64_t	pos;
} __attribute__((__packed__));

static_assert(std::is_pod<SmallNode>::value, "SmallNode must be POD type");

using HashNode = hash::Node;

// ==============================

namespace{
	template<size_t start>
	int comp(Pair const &p, std::string_view const key){
		return p.cmpX<start>(key);
	}

	template<size_t start>
	auto searchBinaryIt_(std::string_view const key, DiskList::random_access_iterator first, DiskList::random_access_iterator last){
		return binarySearch(first, last, key, comp<start>);
	}

	template<size_t start = 0>
	auto searchBinary(std::string_view const key, DiskList const &list, DiskList::size_type const first, DiskList::size_type const last){
		return searchBinaryIt_<start>(key, { list, first }, { list, last });
	}

	auto searchBinary(std::string_view const key, DiskList const &list){
		return searchBinaryIt_<0>(key, list.ra_begin(), list.ra_end());
	}

	// -----------------------------------

	auto searchBTree(std::string_view const key, DiskList const &list) -> BinarySearchResult<DiskList::random_access_iterator>;

	auto searchHotLine(std::string_view const key, DiskList const &list, BlobView const vLine) -> BinarySearchResult<DiskList::random_access_iterator>{
		auto fallback = [key, &list](){
			return searchBinary(key, list);
		};

		size_t const nodesCount = vLine.size() ;

		const SmallNode *nodes = vLine.as<const SmallNode>(0, nodesCount);

		if (!nodes)
			return fallback();

		HPair::HKey const hkey = HPair::SS::create(key);

		auto comp = [](SmallNode const &node, HPair::HKey const hkey){
			const auto [ok, result] = HPair::SS::compare(
					betoh(node.key), // value is in big endian
					hkey
			);

			return result;
		};

		// first try to locate the partition
		auto const nodesEnd = nodes + nodesCount;
		const auto &[found, it] = binarySearch(nodes, nodesEnd, hkey, comp);

		if (it == nodesEnd){
			logger<Logger::DEBUG>() << "Not found, in most right pos";

			return { false, list.ra_end() };
		}

		// Return type fix clang error on MacOS
		DiskList::size_type const listPos = betoh<uint64_t>(it->pos);

		if ( listPos >= list.size() ){
			logger<Logger::ERROR>() << "Hotline corruption detected. Advice Hotline removal.";

			return fallback();
		}

		if (found == false){
			logger<Logger::DEBUG>()
				<<	"Not found"
				<<	"pos" << listPos
				<<	"key" << HPair::toStringView(it->key, std::false_type{})
			;

			return { false,  { list, listPos } };
		}

		// OK, is found in the hot line...

		// if size of the key is less, then it is found as direct hit.

		if (key.size() < HPair::N){
			// if key.size() == HPair::HKey,
			// this does not mean that key is found...

			logger<Logger::DEBUG>() << "Found, direct hit at pos" << listPos;

			return { true, { list, listPos } };
		}

		if (key.size() == HPair::N){
			// key could be there

			bool const found = list[listPos].equalsX<HPair::N>(key);

			logger<Logger::DEBUG>() << "Probing value at pos" << listPos << (found ? "Found" : "Not Found");

			return { found, { list, listPos } };
		}

		// binary inside the partition

		// at this point, we know ALL keys are with same prefix and
		// their size is at least HPair::N
		// this means, we can skip comparing first HPair::N characters

		auto listPosLast = it + 1 == nodesEnd ? list.size() : betoh<uint64_t>( (it + 1)->pos);

		logger<Logger::DEBUG>()
			<<	"Proceed with Binary Search with same prefix" << listPos << listPosLast
			<<	"Hotline Key prefix" << HPair::toStringView(it->key, std::false_type{})
		;

		return searchBinary<HPair::N>(key, list, listPos, listPosLast);
	}

	auto searchHashIndex(std::string_view const key, DiskList const &list, BlobView const vHashIndex) -> BinarySearchResult<DiskList::random_access_iterator>{
		auto fallback = [key, &list](){
			return list.ra_find_<DiskList::FindMode::HASH_FALLBACK>(key);
		};

		size_t const nodesCount = vHashIndex.sizeAs<HashNode>();

		const HashNode *nodes = vHashIndex.as<const HashNode>(0, nodesCount);

		if (!nodes)
			return fallback();

		auto const hc    = murmur_hash64a(key);
		auto const be_hc = htobe(hc);

		for(size_t i = 0; i < hash::HASHTABLE_OPEN_ADDRESSES; ++i){
			// branchless
			auto const cell = (hc + i) % nodesCount;

			auto const &node = nodes[cell];

			if (node.hash == 0){
				logger<Logger::DEBUG>() << "Not found, the open range is terminated.";

				return { false, list.ra_end() };
			}

			if (node.hash == be_hc){
				auto const listPos = betoh(node.pos);

				if ( listPos >= list.size() ){
					logger<Logger::ERROR>() << "HashIndex corruption detected. Advice HashIndex removal.";

					return fallback();
				}

				if (!list[listPos].cmp(key)){
					logger<Logger::DEBUG>() << "Found, direct hit at pos" << listPos;

					return { true, { list, listPos } };
				}

				logger<Logger::DEBUG>() << "Collision on pos" << listPos;
			}
		}

		logger<Logger::DEBUG>() << "Not Found, the open range if full.";

		return fallback();
	}

	// -----------------------------------

	[[maybe_unused]]
	void log__mmap_file__(std::string_view const filename, bool const mmap){
		logger<Logger::NOTICE>() << "mmap" << filename << (mmap ? "Success" : "Error");
	}

	// -----------------------------------

	bool checkMetadata(FileMeta const &metadata){
		if (metadata == false)
			return false;

		// Non sorted files are no longer supported
		if (metadata.sorted() == false){
			logger<Logger::FATAL>() << "Non sorted files are no longer supported. Please replay the file as binlog.";
			return false;
		}

		if (metadata.size() == 0){
			logger<Logger::WARNING>() << "Skipping empty disktable.";
			return false;
		}

		return true;
	}

} // anonymous namespace



// ==============================


//bool DiskList::openDataOnly_(std::string_view const filename, OpenController oc){
//	return oc.open(mData_, filenameData(filename), true);
//}

bool DiskList::openForRepair_(std::string_view const filename, bool const aligned, OpenController oc){
	logger<Logger::WARNING>() << "Open disktable for repair" << filename;

	aligned_ = aligned;

	return oc.open(mData_, filenameData(filename), true);
}

bool DiskList::openForward_(std::string_view const filename, OpenController oc){
	logger<Logger::NOTICE>() << "Open disktable forward only" << filename << "ID" << id_;

	metadata_.open(filenameMeta_string_view(filename));

	if (checkMetadata(metadata_) == false)
		return false;

	bool const result = oc.open(mData_, filenameData(filename), true);

	if (!result)
		metadata_.clear();

	return result;
}

bool DiskList::openMinimal_(std::string_view const filename, OpenController oc){
	logger<Logger::NOTICE>() << "Open disktable" << filename << "ID" << id_;

	metadata_.open(filenameMeta_string_view(filename));

	if (checkMetadata(metadata_) == false)
		return false;

	bool const b1 =	oc.open(mData_, filenameData(filename), true);
	bool const b2 =	oc.open(mIndx_, filenameIndx(filename));

	// integrity check, size is safe to be used now.
	bool const b3 =	BlobView{ mIndx_ }.sizeAs<uint64_t>() == size();

	log__mmap_file__(filenameIndx(filename), mIndx_);
	log__mmap_file__(filenameData(filename), mData_);

	aligned_ = metadata_.aligned();

	bool const result =  b1 && b2 && b3;

	if (!result)
		metadata_.clear();

	return result;
}

bool DiskList::openNormal_ (std::string_view const filename, OpenController oc){
	if (!openMinimal_(filename, oc))
		return false;

	logger<Logger::NOTICE>() << "Open additional files";

	// ==============================

	if (oc.open(mLine_, filenameLine(filename))){
		if (BlobView{ mLine_ }.sizeAs<SmallNode>() <= 1){
			logger<Logger::WARNING>() << "Hotline too small. Ignoring.";
			mLine_.close();
		}
	}

	// ==============================

	if (oc.open(mHash_, filenameHash(filename))){
		// why 64 ? because if less, Line will be as fast as Hash
		if (BlobView{ mHash_ }.sizeAs<HashNode>() < 64){
			logger<Logger::WARNING>() << "HashIndex too small. Ignoring.";
			mHash_.close();
		}
	}

	// ==============================

	if (oc.open(mTree_, filenameBTreeIndx(filename))){
		// open tree keys too
		oc.open(mKeys_, filenameBTreeData(filename));
	}

	log__mmap_file__(filenameLine(filename), mLine_);
	log__mmap_file__(filenameHash(filename), mHash_);

	log__mmap_file__(filenameBTreeIndx(filename), mTree_);
	log__mmap_file__(filenameBTreeData(filename), mKeys_);

	return true;
}

bool DiskList::open_(std::string_view const filename, OpenMode mode, OpenController oc){

	auto f = [&](std::string_view const filename, OpenMode mode, OpenController oc){
		switch(mode){
		default:
		case OpenMode::NORMAL	: return openNormal_  (filename, oc);
		case OpenMode::MINIMAL	: return openMinimal_ (filename, oc);
		case OpenMode::FORWARD	: return openForward_ (filename, oc);
		}
	};

	bool const result = f(filename, mode, oc);
	calcSearchMode_();
	return result;
}

void DiskList::close(){
	if (mData_)
		logger<Logger::NOTICE>() << "Close disktable" << "ID" << id_;

	mIndx_.close();
	mData_.close();

	mLine_.close();
	mHash_.close();

	mTree_.close();
	mKeys_.close();
}



// ==============================



void DiskList::calcSearchMode_(){
	auto bit = [this]{
		uint64_t x = 0;

		x |= mLine_		? 0x001 : 0x0;
		x |= mHash_		? 0x010 : 0x0;
		x |= mTree_ && mKeys_	? 0x100 : 0x0;

		return x;
	};

	auto set = [this](auto t, auto f){
		searchMode_T_ = t;
		searchMode_F_ = f;
	};

	using sm = SearchMode;

	switch( bit() ){
	default:
	case 0x000:	return set(sm::BINARY		, sm::BINARY	);
	case 0x001:	return set(sm::HOTLINE		, sm::HOTLINE	);
	case 0x010:	return set(sm::HASHINDEX	, sm::BINARY	);
	case 0x011:	return set(sm::HASHINDEX	, sm::HOTLINE	);
	case 0x100:
	case 0x101:	return set(sm::BTREE		, sm::BTREE	);
	case 0x110:
	case 0x111:	return set(sm::HASHINDEX	, sm::BTREE	);
	}
}



// ==============================



namespace fd_impl_{
	size_t alignedSize__(const Pair *blob, bool const aligned){
		size_t const size = blob->bytes();

		return ! aligned ? size : my_align::calc(size, PairConf::ALIGN);
	}

	template<bool B>
	const Pair *fdSafeAccess(BlobView const vData, const Pair *blob){
		if (!blob)
			return nullptr;

		// check for overrun, because Pair might be not complete...
		if constexpr(B)
		if (! vData.safeAccessMemory(blob, sizeof(Pair)) )
			return nullptr;

		// check for overrun, because Pair is dynamic size...
		if (! vData.safeAccessMemory(blob, blob->bytes()) )
			return nullptr;

		return blob;
	}

	const Pair *fdGetFirst(BlobView const vData){
		size_t const offset = 0;

		const Pair *blob = vData.as<const Pair>(offset);

		// check for overrun because PairBlob is dynamic size
		return fdSafeAccess<false>(vData, blob);
	}

	const Pair *fdGetNext(BlobView const vData, const Pair *current, bool const aligned){
		size_t size = alignedSize__(current, aligned);

		const char *currentC = (const char *) current;

		const Pair *blob = vData.as<const Pair>(currentC + size);

		// check for overrun because PairBlob is dynamic size
		return fdSafeAccess<false>(vData, blob);
	}

	const Pair *fdGetAt(BlobView const vData, BlobView const vIndx, size_type const index){
		const uint64_t *be_array = vIndx.as<const uint64_t>(0);

		if (!be_array)
			return nullptr;

		uint64_t const be_ptr = be_array[index];

		size_t const offset = narrow<size_t>(betoh<uint64_t>(be_ptr));

		const Pair *blob = vData.as<const Pair>(offset);

		// check for overrun because PairBlob is dynamic size
		return fdSafeAccess<false>(vData, blob);
	}
}



// ==============================


template<DiskList::FindMode B>
auto DiskList::ra_find_(std::string_view const key) const -> BinarySearchResult<random_access_iterator>{
	auto log = [](SearchMode search){
		auto f = []{
			switch(B){
			case FindMode::HASH_FALLBACK	:
			case FindMode::EXACT		: return "Exact";
			case FindMode::PREFIX		: return "Prefix";
			}
		};

		auto s = [search]{
			switch(search){
			default:
			case SearchMode::BINARY		: return "binary";
			case SearchMode::HOTLINE	: return "hotline";
			case SearchMode::HASHINDEX	: return "hashindex";
			case SearchMode::BTREE		: return "btree";
			}
		};

		logger<Logger::DEBUG>() << f() << "search using" << s();
	};



	if constexpr(B == FindMode::PREFIX){
		auto const searchMode = searchMode_F_;

		switch(searchMode){
		default:
		case SearchMode::BINARY		: log(searchMode); return searchBinary	(key, *this);
		case SearchMode::HOTLINE	: log(searchMode); return searchHotLine	(key, *this, mLine_);
		case SearchMode::BTREE		: log(searchMode); return searchBTree	(key, *this);
		}
	}

	if constexpr(B == FindMode::EXACT){
		auto const searchMode = searchMode_T_;

		switch(searchMode){
		default:
		case SearchMode::BINARY		: log(searchMode); return searchBinary		(key, *this);
		case SearchMode::HOTLINE	: log(searchMode); return searchHotLine		(key, *this, mLine_);
		case SearchMode::HASHINDEX	: log(searchMode); return searchHashIndex	(key, *this, mHash_);
		case SearchMode::BTREE		: log(searchMode); return searchBTree		(key, *this);
		}
	}

	if constexpr(B == FindMode::HASH_FALLBACK){
		auto const searchMode = searchMode_F_;

		// [[maybe_unused]]
		// std::string_view const what = "Exact";

		switch(searchMode){
		default:
		case SearchMode::BINARY		: log(searchMode); return searchBinary	(key, *this);
		case SearchMode::HOTLINE	: log(searchMode); return searchHotLine	(key, *this, mLine_);
		case SearchMode::BTREE		: log(searchMode); return searchBTree	(key, *this);
		}
	}
}

auto DiskList::ra_find(std::string_view const key) const -> random_access_iterator{
	auto const result = ra_find_<FindMode::PREFIX>(key);

	return result.it;
}

const Pair *DiskList::getPair(std::string_view const key) const{
	auto const result = ra_find_<FindMode::EXACT>(key);

	return result.found ? & *result.it : nullptr;
}



// ==============================



class DiskList::BTreeSearchHelper{
private:
	using Node				= btree::Node;
	using NodeData				= btree::NodeData;

	using level_type			= btree::level_type;

	using size_type				= DiskList::size_type;

private:
	constexpr static level_type VALUES	= btree::VALUES;

private:
	class BTreeAccessError : std::exception{};

	struct NodeResult{
		bool			isResult;

		// will not fail if level_type is same as size_type
		union{
			level_type	node_index;
			size_type	result_index;
		};

	public:
		constexpr NodeResult(std::true_type,  size_type  const result_index) : isResult(true),  result_index(result_index){}
		constexpr NodeResult(std::false_type, level_type const node_index  ) : isResult(false), node_index(node_index){}
	};

	struct NodeValueResult{
		std::string_view	key;
		size_type		dataid;
	};

private:
	const DiskList		&list_;

	const std::string_view	key_;

	const BlobView		mTree_		= list_.mTree_;
	const BlobView		mKeys_		= list_.mKeys_;

private:
	size_type		start		= 0;
	size_type		end		= list_.size();

public:
	BTreeSearchHelper(const DiskList &list, std::string_view key) : list_(list), key_(key){}

	auto operator()(){
		try{
			return btreeSearch_();
		}catch(const BTreeAccessError &e){
			logger<Logger::DEBUG>() << "Problem, switch to binary search (1)";
			return searchBinary(key_, list_);
		}
	}

private:
	auto btreeSearch_() -> BinarySearchResult<DiskList::random_access_iterator>{
		size_t const nodesCount	= mTree_.size() / sizeof(Node);

		const Node *nodes = mTree_.as<const Node>(0, nodesCount);

		if (!nodes)
			throw BTreeAccessError{};

		size_t pos = 0;

		logger<Logger::DEBUG>() << "BTREE at " << pos;

		while(pos < nodesCount){
			const auto x = levelOrderBinarySearch_( nodes[pos] );

			if (x.isResult)
				return { true, DiskList::random_access_iterator{ list_, x.result_index } };

			// Determine where to go next...

			level_type const branches = VALUES + 1;

			if ( x.node_index == VALUES ){
				// Go Right
				pos = pos * branches + branches;

				logger<Logger::DEBUG>() << "BTREE R:" << pos;
			}else{
				// Go Left
				pos = pos * branches + x.node_index + 1;

				logger<Logger::DEBUG>() << "BTREE L:" << pos << ", node branch" << x.node_index;
			}
		}

		// leaf or similar
		// fallback to binary search :)

		logger<Logger::DEBUG>() << "BTREE LEAF:" << pos;
		logger<Logger::DEBUG>() << "Fallback to binary search" << start << '-' << end << ", width" << (end - start);

		return searchBinary(key_, list_, start, end);
	}

	NodeResult levelOrderBinarySearch_(const Node &node){
		const auto &ll = btree::LL;

		level_type node_pos = 0;

		level_type node_index;

		do{
			const auto x = accessNodeValue_( node.values[node_pos] );

			logger<Logger::DEBUG>() << "\tNode Value" << node_pos << "[" << ll[node_pos] << "], Key:" << x.key;

			int const cmp = x.key.compare(key_);

			if (cmp < 0){
				// this + 1 is because,
				// we want if possible to go LEFT instead of RIGHT
				// node_index will go out of bounds,
				// this is indicator for RIGHT
				node_index = level_type(ll[node_pos] + 1);

				// go right
				node_pos = level_type(2 * node_pos + 1 + 1);

				start = x.dataid + 1;

				logger<Logger::DEBUG>() << "\t\tR:" << node_pos << "BS:" << start << '-' << end;
			}else if (cmp > 0){
				node_index = ll[node_pos];

				// go left
				node_pos = level_type(2 * node_pos + 1);

				end = x.dataid;

				logger<Logger::DEBUG>() << "\t\tL:" << node_pos << "BS:" << start << '-' << end;
			}else{
				// found

				logger<Logger::DEBUG>() << "\t\tFound at" << node_pos;

				return { std::true_type{}, x.dataid };
			}
		}while (node_pos < VALUES);

		return { std::false_type{}, node_index };
	}

	NodeValueResult accessNodeValue_(uint64_t const offsetBE) const{
		uint64_t const offset = betoh<uint64_t>( offsetBE );

		// BTree NIL case - can not happen

		const NodeData *nd = mKeys_.as<const NodeData>(narrow<size_t>(offset));

		if (!nd)
			throw BTreeAccessError{};

		size_t    const keysize = betoh<uint16_t>(nd->keysize);
		size_type const dataid  = betoh<uint64_t>(nd->dataid);

		// key is just after the NodeData
		const char *keyptr = mKeys_.as<const char>(narrow<size_t>(offset + sizeof(NodeData)), keysize);

		if (!keyptr)
			throw BTreeAccessError{};

		return { { keyptr, keysize }, dataid };
	}
};



namespace{

	auto searchBTree(std::string_view const key, DiskList const &list) -> BinarySearchResult<DiskList::random_access_iterator>{
		DiskList::BTreeSearchHelper helper{list, key};

		return helper();
	}

} // namespace

} // namespace disk
} // namespace


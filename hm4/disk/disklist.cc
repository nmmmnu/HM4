#include "disklist.h"
#include "hpair.h"

#include "disk/filenames.h"
#include "disk/btreeindexnode.h"

#include "stringhash.h"

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

// ==============================

namespace{

	template<bool B>
	auto find_fix(BinarySearchResult<DiskList::random_access_iterator> const result, DiskList const &list, std::bool_constant<B>){
		if constexpr(B)
			return result.found ? result.it : list.ra_end();
		else
			return result.it;

	}

	// -----------------------------------

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

	template<size_t start = 0>
	auto searchBinary(std::string_view const key, DiskList const &list){
		return searchBinaryIt_<start>(key, list.ra_begin(), list.ra_end());
	}

	// -----------------------------------

	auto searchBTree(std::string_view const key, DiskList const &list) -> BinarySearchResult<DiskList::random_access_iterator>;

	auto searchHotLine(std::string_view const key, DiskList const &list, MMAPFilePlus const &line) -> BinarySearchResult<DiskList::random_access_iterator>{
		size_t const nodesCount = line.sizeArray<SmallNode>();

		const SmallNode *nodes = line->as<const SmallNode>(0, nodesCount);

		if (!nodes)
			return searchBinary(key, list);

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

			return searchBinary(key, list);
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

	[[maybe_unused]]
	void log__mmap_file__(std::string_view const filename, bool const mmap){
		logger<Logger::NOTICE>() << "mmap" << filename << (mmap ? "Success" : "Error");
	}

} // anonymous namespace



// ==============================


bool DiskList::openDataOnly_(std::string_view const filename){
	MMAPFile::Advice const advice = MMAPFile::Advice::SEQUENTIAL;

	return mData_.open(filenameData(filename), advice);
}

bool DiskList::openDataOnly_with_bool(std::string_view const filename, bool const aligned){
	logger<Logger::WARNING>() << "Open disktable for repair" << filename;

	aligned_ = aligned;

	return openDataOnly_(filename);
}



namespace{
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
}



bool DiskList::openForward_(std::string_view const filename){
	logger<Logger::NOTICE>() << "Open disktable forward only" << filename << "ID" << id_;

	metadata_.open(filenameMeta_string_view(filename));

	if (checkMetadata(metadata_) == false)
		return false;

	bool const result = openDataOnly_(filename);

	if (!result)
		metadata_.clear();

	return result;
}

bool DiskList::openMinimal_(std::string_view const filename, MMAPFile::Advice const advice){
	logger<Logger::NOTICE>() << "Open disktable" << filename << "ID" << id_;

	metadata_.open(filenameMeta_string_view(filename));

	if (checkMetadata(metadata_) == false)
		return false;

	bool const b1 =	mIndx_.open(filenameIndx(filename));
	bool const b2 =	mData_.open(filenameData(filename), advice);

	// integrity check, size is safe to be used now.
	bool const b3 =	mIndx_.sizeArray<uint64_t>() == size();

	log__mmap_file__(filenameIndx(filename), mIndx_);
	log__mmap_file__(filenameData(filename), mData_);

	aligned_ = metadata_.aligned();

	bool const result =  b1 && b2 && b3;

	if (!result)
		metadata_.clear();

	return result;
}

bool DiskList::openNormal_(std::string_view const filename, MMAPFile::Advice const advice){
	if (openMinimal_(filename, advice) == false)
		return false;

	logger<Logger::NOTICE>() << "Open additional files";

	// ==============================

	mLine_.open(filenameLine(filename));

	if (mLine_.sizeArray<SmallNode>() <= 1){
		logger<Logger::WARNING>() << "Hotline too small. Ignoring.";
		mLine_.close();
	}

	// ==============================

	mTree_.open(filenameBTreeIndx(filename));
	mKeys_.open(filenameBTreeData(filename));

	log__mmap_file__(filenameLine(filename), mLine_);

	log__mmap_file__(filenameBTreeIndx(filename), mTree_);
	log__mmap_file__(filenameBTreeData(filename), mKeys_);

	return true;
}

bool DiskList::open_(std::string_view const filename, MMAPFile::Advice const advice, OpenMode const mode){
	// this can not be converted to lambda easily...
	switch(mode){
	default:
	case OpenMode::NORMAL	: return openNormal_  (filename, advice	);
	case OpenMode::MINIMAL	: return openMinimal_ (filename, advice	);
	case OpenMode::FORWARD	: return openForward_ (filename		);
	}
}

bool DiskList::open(std::string_view const filename, MMAPFile::Advice const advice, OpenMode const mode){
	bool const result = open_(filename, advice, mode);
	searchMode_ = calcSearchMode_();
	return result;
}

void DiskList::close(){
	if (mData_)
		logger<Logger::NOTICE>() << "Close disktable" << "ID" << id_;

	mIndx_.close();
	mData_.close();

	mLine_.close();

	mTree_.close();
	mKeys_.close();
}



// ==============================



auto DiskList::calcSearchMode_() const -> SearchMode{
	if (mTree_ && mKeys_)
		return SearchMode::BTREE;

	if (mLine_)
		return SearchMode::HOTLINE;

	else
		return SearchMode::BINARY;
}



// ==============================



namespace fd_impl_{
	size_t alignedSize__(const Pair *blob, bool const aligned){
		size_t const size = blob->bytes();

		return ! aligned ? size : my_align::calc(size, PairConf::ALIGN);
	}

	template<bool B>
	const Pair *fdSafeAccess(BlobRef const &mData, const Pair *blob){
		if (!blob)
			return nullptr;

		// check for overrun, because Pair might be not complete...
		if constexpr(B)
		if (! mData.safeAccessMemory(blob, sizeof(Pair)) )
			return nullptr;

		// check for overrun, because Pair is dynamic size...
		if (! mData.safeAccessMemory(blob, blob->bytes()) )
			return nullptr;

		return blob;
	}

	const Pair *fdGetFirst(BlobRef const &mData){
		size_t const offset = 0;

		const Pair *blob = mData.as<const Pair>(offset);

		// check for overrun because PairBlob is dynamic size
		return fdSafeAccess<false>(mData, blob);
	}

	const Pair *fdGetNext(BlobRef const &mData, const Pair *current, bool const aligned){
		size_t size = alignedSize__(current, aligned);

		const char *currentC = (const char *) current;

		const Pair *blob = mData.as<const Pair>(currentC + size);

		// check for overrun because PairBlob is dynamic size
		return fdSafeAccess<false>(mData, blob);
	}

	const Pair *fdGetAt(BlobRef const &mData, BlobRef const &mIndx, size_type const index){
		const uint64_t *be_array = mIndx.as<const uint64_t>(0);

		if (!be_array)
			return nullptr;

		uint64_t const be_ptr = be_array[index];

		size_t const offset = narrow<size_t>(betoh<uint64_t>(be_ptr));

		const Pair *blob = mData.as<const Pair>(offset);

		// check for overrun because PairBlob is dynamic size
		return fdSafeAccess<false>(mData, blob);
	}
}



// ==============================



template<bool B>
auto DiskList::ra_find(std::string_view const key, std::bool_constant<B> const exact) const -> random_access_iterator{
	// made this way to hide BinarySearchResult<iterator> type
	switch(searchMode_){
	case SearchMode::BTREE: {
			logger<Logger::DEBUG>() << "btree";
			auto const x = searchBTree(key, *this);
			return find_fix(x, *this, exact);
		}

	case SearchMode::HOTLINE: {
			logger<Logger::DEBUG>() << "hotline";
			auto const x = searchHotLine(key, *this, mLine_);
			return find_fix(x, *this, exact);
		}

	default:
	case SearchMode::BINARY: {
			logger<Logger::DEBUG>() << "binary";
			auto const x = searchBinary(key, *this);
			return find_fix(x, *this, exact );
		}
	}
}

template auto DiskList::ra_find(std::string_view const key, std::true_type  ) const -> random_access_iterator;
template auto DiskList::ra_find(std::string_view const key, std::false_type ) const -> random_access_iterator;



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

	const std::string_view	&key_;

	const MMAPFilePlus	&mTree_		= list_.mTree_;
	const MMAPFilePlus	&mKeys_		= list_.mKeys_;

private:
	size_type		start		= 0;
	size_type		end		= list_.size();

public:
	BTreeSearchHelper(const DiskList &list, std::string_view const &key) : list_(list), key_(key){}

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

		const Node *nodes = mTree_->as<const Node>(0, nodesCount);

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

		const NodeData *nd = mKeys_->as<const NodeData>(narrow<size_t>(offset));

		if (!nd)
			throw BTreeAccessError{};

		size_t    const keysize = betoh<uint16_t>(nd->keysize);
		size_type const dataid  = betoh<uint64_t>(nd->dataid);

		// key is just after the NodeData
		const char *keyptr = mKeys_->as<const char>(narrow<size_t>(offset + sizeof(NodeData)), keysize);

		if (!keyptr)
			throw BTreeAccessError{};

		return { { keyptr, keysize }, dataid };
	}
};



// ==============================



namespace{
	auto searchBTree(std::string_view const key, DiskList const &list) -> BinarySearchResult<DiskList::random_access_iterator>{
		DiskList::BTreeSearchHelper helper{list, key};

		return helper();
	}
} // anonymous namespace


} // namespace disk
} // namespace


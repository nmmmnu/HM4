#ifndef DISK_LIST_H_
#define DISK_LIST_H_

#include "mmapfilesbo.h"
#include "blobview.h"

#include "ilist.h"
#include "filemeta.h"

#include <type_traits>



template<class Iterator>
struct BinarySearchResult;



namespace hm4::disk{

namespace fd_impl_{
	using config::size_type;

	const Pair *fdGetFirst	(BlobView const vData);
	const Pair *fdGetNext	(BlobView const vData, const Pair *blob, bool aligned);

	const Pair *fdGetAt	(BlobView const vData, BlobView const vIndx, size_type index);
}



class DiskList{
public:
	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

	class random_access_iterator;
	class forward_iterator;

	using iterator = forward_iterator;

	using SlabAllocator = MMAPFileSBO::SlabAllocator;

public:
	enum class OpenMode : char {
		NORMAL		,
		MINIMAL		,
		FORWARD
	};

	enum class SearchMode : char {
		BINARY,
		HOTLINE,
		HASHINDEX,
		BTREE
	};

	static constexpr MMAPFile::Advice	DEFAULT_ADVICE	= MMAPFile::Advice::RANDOM;
	static constexpr OpenMode		DEFAULT_MODE	= OpenMode::NORMAL;

private:
	static constexpr size_type	BIN_SEARCH_MINIMUM_DISTANCE	= 3;
//	static constexpr int		CMP_ZERO			= 1;

public:
	DiskList() = default;

	DiskList(DiskList &&other) = default;

	// no need d-tor,
	// MMAPFile-s will be closed automatically
	// However, we wanted printed message
	~DiskList(){
		close();
	}

	bool open(uint64_t id, std::string_view filename, MMAPFile::Advice advice = DEFAULT_ADVICE, OpenMode mode = DEFAULT_MODE){
		id_ = id;

		return open(filename, advice, mode);
	}

	bool open(std::string_view filename, MMAPFile::Advice advice = DEFAULT_ADVICE, OpenMode mode = DEFAULT_MODE);

	bool openDataOnly_with_bool(std::string_view filename, bool aligned);

	bool openDataOnly(std::string_view filename, hm4::Pair::WriteOptions writeOptions){
		auto translateOptions = [](auto x){
			switch(x){
			case hm4::Pair::WriteOptions::ALIGNED	: return true;
			case hm4::Pair::WriteOptions::NONE	: return false;
			}
		};

		return openDataOnly_with_bool(filename, translateOptions(writeOptions) );
	}

	void close();

	void printMetadata() const{
		metadata_.print();
	}

	void setSBOAllocator(SlabAllocator &allocator){
		sboAllocator_ = &allocator;
	}

public:
	Pair const &operator[](size_type const index) const{
		assert( index < size() );

		return *fdGetAt_(index);
	}

	size_type size() const{
		return metadata_.size();
	}

	constexpr static bool conf_always_non_empty	= true;

	constexpr bool empty() const{
		return false;
	}

	size_t bytes() const{
		return mData_.size();
	}

	bool aligned() const{
		return aligned_;
	}

	auto id() const{
		return id_;
	}

	auto createdMin() const{
		return metadata_.createdMin();
	}

	auto createdMax() const{
		return metadata_.createdMax();
	}

public:
	constexpr static bool conf_no_crontab	= true;

	constexpr static void crontab(){
	}

public:
	random_access_iterator ra_begin() const;
	random_access_iterator ra_end() const;

	template<bool B>
	random_access_iterator ra_find(std::string_view const key, std::bool_constant<B>) const;

	enum class FindMode{
		EXACT,
		PREFIX,
		HASH_FALLBACK
	};

	template<FindMode B>
	BinarySearchResult<random_access_iterator> ra_find_(std::string_view const key) const;

private:
	forward_iterator make_forward_iterator_(const Pair *pair) const;

public:
	forward_iterator begin() const;
	constexpr forward_iterator end() const;

	template<bool B>
	forward_iterator find(std::string_view key, std::bool_constant<B>) const;

private:
	bool openNormal_  (std::string_view filename, MMAPFile::Advice advice);
	bool openMinimal_ (std::string_view filename, MMAPFile::Advice advice);
	bool openForward_ (std::string_view filename);

	bool openDataOnly_(std::string_view filename);

	bool open_(std::string_view filename, MMAPFile::Advice advice, OpenMode mode);

	void calcSearchMode_();

private:
	const Pair *fdGetAt_(size_type const index) const{
		return fd_impl_::fdGetAt(mData_, mIndx_, index);
	}

	const Pair *fdGetFirst_() const{
		return fd_impl_::fdGetFirst(mData_);
	}

public:
	class BTreeSearchHelper;

private:
	MMAPFileSBO	mIndx_;
	MMAPFileSBO	mLine_;
	MMAPFileSBO	mHash_;
	MMAPFileSBO	mData_;

	MMAPFileSBO	mTree_;
	MMAPFileSBO	mKeys_;

	FileMeta	metadata_;

	SearchMode	searchMode_T_	= SearchMode::BINARY;
	SearchMode	searchMode_F_	= SearchMode::BINARY;

	bool		aligned_	= true;

	uint64_t	id_		= 0;

	SlabAllocator	*sboAllocator_	= nullptr;
};



} // namespace disk



#include "disklist.iterator.cc.h"



namespace hm4::disk{

	inline auto DiskList::ra_begin() const -> random_access_iterator{
		return { *this, difference_type{ 0 } };
	}

	inline auto DiskList::ra_end() const -> random_access_iterator{
		return { *this, size() };
	}

	inline auto DiskList::make_forward_iterator_(const Pair *pair) const -> forward_iterator{
		return forward_iterator(mData_, pair, aligned());
	}

	inline auto DiskList::begin() const -> forward_iterator{
		return make_forward_iterator_( fdGetFirst_() );
	}

	constexpr inline auto DiskList::end() const -> forward_iterator{
		return {};
	}

	template<bool B>
	auto DiskList::find(std::string_view const key, std::bool_constant<B> const exact) const -> forward_iterator{
		// gcc error if "forward_iterator" ommited
		return forward_iterator{ ra_find(key, exact) };
	}

} // namespace disk

#endif


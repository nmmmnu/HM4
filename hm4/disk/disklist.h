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
	constexpr static bool USE_SBO = true;

public:
	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

	class random_access_iterator;
	class forward_iterator;

	using iterator = forward_iterator;

	using VMAllocator	= MMAPFileSBO::VMAllocator;
	using NoVMAllocator	= MMAPFileSBO::NoVMAllocator;

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

	static constexpr MMAPFile::Advice	DEFAULT_ADVICE_	= MMAPFile::Advice::RANDOM;
	static constexpr OpenMode		DEFAULT_MODE	= OpenMode::NORMAL;

private:
	static constexpr size_type	BIN_SEARCH_MINIMUM_DISTANCE	= 3;
//	static constexpr int		CMP_ZERO			= 1;

	constexpr static auto calcAdvice__(OpenMode mode){
		switch(mode){
		default:
		case OpenMode::NORMAL	:
		case OpenMode::MINIMAL	: return DEFAULT_ADVICE_		;
		case OpenMode::FORWARD	: return MMAPFile::Advice::SEQUENTIAL	;
		}
	}

public:
	DiskList() = default;

	DiskList(DiskList &&other) = default;

	// no need d-tor,
	// MMAPFile-s will be closed automatically
	// However, we wanted printed message
	~DiskList(){
		close();
	}

	struct OpenController{
		MMAPFile::Advice	advice		= DEFAULT_ADVICE_;
		VMAllocator		*allocator	= nullptr;

		bool open(MMAPFileSBO &f, std::string_view filename, bool b = false){
			return f.open(allocator, filename, adviceX_(b));
		}

		bool open(MMAPFile &f, std::string_view filename, bool b = false){
			return f.open(filename, adviceX_(b));
		}

	private:
		constexpr MMAPFile::Advice adviceX_(bool b) const{
			return b ? advice : DEFAULT_ADVICE_;
		}
	};

	bool open(uint64_t id, std::string_view filename, VMAllocator *allocator, OpenMode mode = DEFAULT_MODE){
		id_ = id;

		auto const advice = calcAdvice__(mode);

		return open_(filename, mode, OpenController{ advice, allocator });
	}

	bool open(uint64_t id, std::string_view filename, NoVMAllocator,          OpenMode mode = DEFAULT_MODE){
		return open(id, filename, nullptr,    mode);
	}

	bool open(uint64_t id, std::string_view filename, VMAllocator &allocator, OpenMode mode = DEFAULT_MODE){
		return open(id, filename, &allocator, mode);
	}

	bool open(std::string_view filename, NoVMAllocator,          OpenMode mode = DEFAULT_MODE){
		return open( 0, filename, nullptr,    mode);
	}

	bool open(std::string_view filename, VMAllocator &allocator, OpenMode mode = DEFAULT_MODE){
		return open( 0, filename, &allocator, mode);
	}

	bool openForRepair(std::string_view filename, bool aligned){
		auto const advice = MMAPFile::Advice::SEQUENTIAL;

		return openForRepair_(filename, aligned, OpenController{ advice });
	}

	bool openForRepair(std::string_view filename, bool aligned, VMAllocator &allocator){
		auto const advice = MMAPFile::Advice::SEQUENTIAL;

		return openForRepair_(filename, aligned, OpenController{ advice, &allocator });
	}

	void close();

	void printMetadata() const{
		metadata_.print();
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
	bool openNormal_	(std::string_view filename, OpenController oc);
	bool openMinimal_	(std::string_view filename, OpenController oc);
	bool openForward_	(std::string_view filename, OpenController oc);

	bool openForRepair_	(std::string_view filename, bool aligned,  OpenController oc);
	bool open_		(std::string_view filename, OpenMode mode, OpenController oc);

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
	using MMAPFileCond = std::conditional_t<
				USE_SBO,
				MMAPFileSBO,
				MMAPFile
	>;

	MMAPFileCond	mIndx_;
	MMAPFileCond	mLine_;
	MMAPFileCond	mHash_;
	MMAPFileCond	mData_;

	MMAPFileCond	mTree_;
	MMAPFileCond	mKeys_;

	FileMeta	metadata_;

	SearchMode	searchMode_T_	= SearchMode::BINARY;
	SearchMode	searchMode_F_	= SearchMode::BINARY;

	bool		aligned_	= true;

	uint64_t	id_		= 0;
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


#ifndef DISK_LIST_H_
#define DISK_LIST_H_

#include "mmapfileplus.h"

#include "ilist.h"
#include "filemeta.h"

#include <type_traits>

namespace hm4{
namespace disk{



class DiskList{
public:
	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

	class random_access_iterator;
	class forward_iterator;

	using iterator = forward_iterator;

public:
	enum class OpenMode : char {
		NORMAL		,
		MINIMAL		,
		DATA_ONLY
	};

	enum class SearchMode : char {
		BINARY,
		HOTLINE,
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
	// ~DiskList() = default;

	bool open(std::string_view filename, MMAPFile::Advice advice = DEFAULT_ADVICE, OpenMode mode = DEFAULT_MODE);
	void close();

	operator bool(){
		return metadata_;
	}

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

	size_t bytes() const{
		return mData_.size();
	}

	bool sorted() const{
		return metadata_.sorted();
	}

	bool aligned() const{
		return metadata_.aligned();
	}

public:
	random_access_iterator ra_begin() const;
	random_access_iterator ra_end() const;

	template<bool B>
	random_access_iterator ra_find(std::string_view const key, std::bool_constant<B>) const;

	forward_iterator beginFromFirst() const;

	forward_iterator begin() const;
	forward_iterator end() const;

	template<bool B>
	forward_iterator find(std::string_view key, std::bool_constant<B>) const;

private:
	bool openNormal_  (std::string_view filename, MMAPFile::Advice advice);
	bool openMinimal_ (std::string_view filename, MMAPFile::Advice advice);
	bool openDataOnly_(std::string_view filename);

	bool open_(std::string_view filename, MMAPFile::Advice advice, OpenMode mode);

	SearchMode calcSearchMode_() const;

private:
	const Pair *fdSafeAccess_(const Pair *blob) const;

	const Pair *fdGetAtZero_() const{
		return metadata_.sorted() ? fdGetFirstPair_() : fdGetAt_(0);
	}

	const Pair *fdGetAtSafeBound_(size_type const index) const{
		return index < size() ? fdGetAt_(index) : nullptr;
	}

	const Pair *fdGetFirstPair_() const;

	const Pair *fdGetAt_(size_type index) const;
	const Pair *fdGetNext_(const Pair *blob) const;
	static size_t alignedSize__(const Pair *blob, bool aligned);

public:
	class BTreeSearchHelper;

private:
	MMAPFilePlus	mIndx_;
	MMAPFilePlus	mLine_;
	MMAPFilePlus	mData_;

	MMAPFilePlus	mTree_;
	MMAPFilePlus	mKeys_;

	FileMeta	metadata_;

	SearchMode	searchMode_	= SearchMode::BINARY;
};



} // namespace disk
} // namespace



#include "disklist.iterator.cc.h"



namespace hm4{
namespace disk{



inline auto DiskList::ra_begin() const -> random_access_iterator{
	return { *this, difference_type{ 0 } };
}

inline auto DiskList::ra_end() const -> random_access_iterator{
	return { *this, size() };
}

inline auto DiskList::beginFromFirst() const -> forward_iterator{
	return { *this, fdGetFirstPair_() };
}

inline auto DiskList::begin() const -> forward_iterator{
	return { *this, fdGetAtZero_() };
}

inline auto DiskList::end() const -> forward_iterator{
	return {};
}

template<bool B>
auto DiskList::find(std::string_view const key, std::bool_constant<B> const exact) const -> forward_iterator{
	return { ra_find(key, exact) };
}

} // namespace disk
} // namespace

#endif


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
		NORMAL,
		MINIMAL
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

	void close();

	bool open(const StringRef &filename, MMAPFile::Advice const advice = DEFAULT_ADVICE, OpenMode const mode = DEFAULT_MODE){
		switch(mode){
		default:
		case OpenMode::NORMAL	: return openNormal_ (filename, advice);
		case OpenMode::MINIMAL	: return openMinimal_(filename, advice);
		}

		searchMode_ = calcSearchMode_();
	}

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
	random_access_iterator ra_find(const StringRef &key, std::bool_constant<B>) const;

	forward_iterator begin() const;
	forward_iterator end() const;

	template<bool B>
	forward_iterator find(const StringRef &key, std::bool_constant<B>) const;

private:
	bool openNormal_ (const StringRef &filename, MMAPFile::Advice advice);
	bool openMinimal_(const StringRef &filename, MMAPFile::Advice advice);

	SearchMode calcSearchMode_() const;

private:
	const Pair *fdSafeAccess_(const Pair *blob) const;

	const Pair *fdGetAt_(size_type index) const;
	const Pair *fdGetNext_(const Pair *blob) const;
	static size_t alignedSize__(const Pair *blob, bool aligned);

public:
	class BTreeSearchHelper;

private:
	MMAPFilePlus		mIndx_;
	MMAPFilePlus		mLine_;
	MMAPFilePlus		mData_;

	MMAPFilePlus		mTree_;
	MMAPFilePlus		mKeys_;

	FileMeta		metadata_;

	SearchMode		searchMode_	= SearchMode::BINARY;
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

inline auto DiskList::begin() const -> forward_iterator{
	return { *this, difference_type{ 0 } };
}

inline auto DiskList::end() const -> forward_iterator{
	return { *this, size() };
}

template<bool B>
auto DiskList::find(StringRef const &key, std::bool_constant<B> const exact) const -> forward_iterator{
	return static_cast<forward_iterator>( ra_find(key, exact) );
}

} // namespace disk
} // namespace

#endif


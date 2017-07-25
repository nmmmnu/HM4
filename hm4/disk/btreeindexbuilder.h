#ifndef _BTREE_INDEX_BUILDER_H
#define _BTREE_INDEX_BUILDER_H

#include "btreeindexnode.h"
#include "filenames.h"

#include <string>
#include <fstream>

namespace hm4{
namespace disk{
namespace btree{


template <class LIST>
class BTreeIndexBuilder{
public:
	using size_type = typename LIST::size_type;

public:
	BTreeIndexBuilder(std::string filename, const LIST &list) :
							list_(list),
							filename_(std::move(filename)){}

	bool build();

private:
	static level_type calcDepth__(size_type count);

private:
	void reorderLevel_(level_type const target_level){
		reorder_(0, list_.size(), target_level, 0);
	}

	void reorder_(size_type const begin, size_type const end, level_type const target_level, level_type const current_level);

private:
	void push_back_key(size_type index);

	static size_t calcAlign__(size_t const value, uint16_t const align){
		return align * ((value + align - 1) / align);
	}

private:
	const LIST	&list_;

	size_t		current_	= 0;

	std::string	filename_;

	std::ofstream	file_indx_;
	std::ofstream	file_data_;
};


} // namespace btree
} // namespace disk
} // namespace


#include "btreeindexbuilder_impl.h"

#endif


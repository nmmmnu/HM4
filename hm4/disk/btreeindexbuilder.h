#ifndef _BTREE_INDEX_BUILDER_H
#define _BTREE_INDEX_BUILDER_H

#include "btreeindexnode.h"
#include "filenames.h"
#include "filewriter.h"
//#include "mybuffer.h"

#include <string>

namespace hm4{
namespace disk{
namespace btree{


template <class List>
class BTreeIndexBuilder{
public:
	using size_type = typename List::size_type;

public:
	BTreeIndexBuilder(std::string filename, const List &list) :
							list_(list),
							filename_(std::move(filename)){}

	bool build(MyBuffer::ByteBufferView bufferIndx, MyBuffer::ByteBufferView bufferData);

private:
	static level_type calcDepth__(size_type count);

private:
	void reorderLevel_(level_type const target_level){
		reorder_(0, list_.size(), target_level, 0);
	}

	void reorder_(size_type const begin, size_type const end, level_type const target_level, level_type const current_level);

private:
	void push_back_key(size_type index);

	bool aligned_(){
		return list_.aligned();
	}

private:
	const List	&list_;

	size_t		current_	= 0;

	std::string	filename_;

	FileWriter	file_indx_;
	FileWriter	file_data_;
};


} // namespace btree
} // namespace disk
} // namespace


#include "btreeindexbuilder.h.cc"

#endif


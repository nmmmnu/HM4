#include <iostream>
#include <stdexcept>

#include "myendian.h"
#include "myalign.h"
#include "mynarrow.h"


namespace hm4{
namespace disk{
namespace btree{


template <class List>
bool BTreeIndexBuilder<List>::build(){
	size_type const size = list_.size();

	level_type total_levels = level_type(calcDepth__(size) - 1);

	if (total_levels == 0){
		std::cout << "List contains very few elements. Skipping BTree building." << '\n';
		return true;
	}

	file_indx_ = { filenameBTreeIndx(filename_) };
	file_data_ = { filenameBTreeData(filename_) };

	std::cout
		 << "Records          : "	<< size			<< '\n'
		 << "Branching Factor : "	<< int{VALUES}		<< '\n'
		 << "Tree Depth       : "	<< int{total_levels}	<< '\n'
	;

	current_ = 0;

	for(level_type level = 0; level < total_levels; ++level){
		std::cout << "Processing level " << int{level} << "..." << '\n';

		reorderLevel_(level);
	}

	return true;
}


template <class List>
void BTreeIndexBuilder<List>::reorder_(size_type const begin, size_type const end,
					level_type const target_level, level_type const current_level){
	if (begin >= end)
		return;

	size_type const size = end - begin;
	size_type const distance = size / (VALUES + 1);

	if (current_level < target_level){
		level_type const next_level = level_type(current_level + 1);

		size_type b = begin;
		for(level_type i = 0; i < VALUES; ++i){
			size_type e = begin + distance * (i + 1);

			reorder_(b, e, target_level, next_level );

			b = e + 1;
		}

		// last right child
		reorder_(b, end, target_level, next_level );

	}else{ // current_level == target_level

		if (size <= VALUES){
			std::cout << "Node is half full. Something is incorrect." << '\n';
			throw std::logic_error("Node is half full");
		}

		// NORMAL NODE WITH ALL ELEMENTS PRESENT

		Node node;

		for(level_type j = 0; j < VALUES; ++j){
			level_type const i = LL[j];

			node.values[j] = htobe<uint_fast64_t>( current_ );

			size_type const index = begin + distance * (i + 1);

			push_back_key(index);
		}

		// push the node
		file_indx_.write( (const char *) & node, sizeof node );
	}
}

// ==============================

template <class List>
void BTreeIndexBuilder<List>::push_back_key(size_type const index){
	// we need to have the pair,
	// because key "live" inside it.
	auto const &pair = list_[index];
	std::string_view const key = pair.getKey();

	NodeData nd;
	nd.dataid  = htobe<uint64_t>(index);
	nd.keysize = htobe<uint16_t>(narrow<uint16_t>(key.size()));

	// push NodeData
	file_data_.write( (const char *) &nd, sizeof nd );

	// push the key
	file_data_.write( key.data(), (std::streamsize) key.size() );

	// push NULL terminator to help recover the file
	file_data_.put(0);

	size_t const data_size = sizeof(NodeData) + key.size() + 1;

	current_ += data_size;

	// push the align
	if (aligned_())
		current_ += my_align::fwriteGap(file_data_, data_size, NodeData::ALIGN);
}

// ==============================

template <class List>
auto BTreeIndexBuilder<List>::calcDepth__(size_type count) -> level_type{
	// Biliana
	// log 54 (123) = ln (123) / ln (54)
	// but this is true for B+Tree only...

	level_type result = 0;

	while(count > 0){
		++result; // tree is always 1 level.

		if (count > VALUES){
			// We substract VALUES,
			// because BTree have data in non-leaf nodes.
			count = (count - VALUES) / (VALUES + 1);
		}else{
			break;
		}
	}

	return result;
}


} // namespace btree
} // namespace disk
} // namespace


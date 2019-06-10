#ifndef BASE_TABLE_LOADER_H_
#define BASE_TABLE_LOADER_H_

#include "disk/disklist.h"
#include "multi/collectionlist.h"

#include <vector>

namespace hm4{
namespace listloader{
	namespace impl_{


		struct ContainerHelper{
			using DiskList		= hm4::disk::DiskList;
			using Container		= std::vector<DiskList>;
			using CollectionList	= hm4::multi::CollectionList<Container>;

			ContainerHelper(MMAPFile::Advice const advice, DiskList::OpenMode const mode) :
							list_(container_),
							advice_(advice),
							mode_(mode){}

			void clear(){
				container_.clear();
			}

			void reserve(size_t const size){
				container_.reserve(size);
			}

			void push_back(const StringRef &filename){
				container_.emplace_back();
				container_.back().open(filename, advice_, mode_);
			}

			const CollectionList &getList() const{
				return list_;
			}

		private:
			Container		container_;

			CollectionList		list_;

			MMAPFile::Advice	advice_;
			DiskList::OpenMode	mode_;
		};


	} // namespace impl_
} // namespace listloader
} // namespace


#endif


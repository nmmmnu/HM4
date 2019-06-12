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

			const CollectionList &getList() const{
				return list_;
			}

			template<class IT>
			void copy(IT first, IT last, size_t const size){
				auto &c = container_;

				c.clear();
				c.reserve(size);

				for(auto it = std::make_reverse_iterator(last); it != std::make_reverse_iterator(first); ++it){
					auto filename = *it;

					c.emplace_back();
					c.back().open(filename, advice_, mode_);
				}
			}

			template<class IT>
			void copy(IT first, IT last){
				return copy(first, last, narrow<size_t>(std::distance(first, last)));
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


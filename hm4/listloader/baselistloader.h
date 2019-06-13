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

			const auto &getList() const{
				return list_;
			}

			void copy(std::nullptr_t, std::nullptr_t){
				container_.clear();
			}

			template<class IT>
			void copy(IT first, IT last){
				size_t const size = narrow<typename Container::size_type>(std::distance(first, last));

				container_.clear();
				container_.reserve(size);

				for(auto it = std::make_reverse_iterator(last); it != std::make_reverse_iterator(first); ++it)
					push_back_(*it);
			}

		private:
			void push_back_(const char *filename);

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


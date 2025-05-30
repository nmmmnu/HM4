#ifndef BASE_TABLE_LOADER_H_
#define BASE_TABLE_LOADER_H_

#include "disk/disklist.h"
#include "multi/collectionlist.h"
#include "myfs.h"

#include <vector>
#include <string_view>



namespace hm4{
namespace listloader{

	namespace impl_{

		struct ContainerHelper{
			using DiskList		= hm4::disk::DiskList;
			using Container		= std::vector<DiskList>;
			using CollectionList	= hm4::multi::CollectionList<Container>;

			ContainerHelper(DiskList::VMAllocator *allocator, DiskList::OpenMode const mode) :
							allocator_	(allocator	),
							mode_		(mode		){}

			const auto &getList() const{
				return list_;
			}

			void copy(){
				container_.clear();
			}

			template<class IT>
			void copy(IT first, IT last){
				size_t const size = narrow<typename Container::size_type>(std::distance(first, last));

				Container neo;
				neo.reserve(size);

				for(auto it = first; it != last; ++it){
					std::string_view filename = *it;

					auto const id = checkFileInode(filename);

					if (id == 0)
						continue;

					auto f = [id](auto const &table){
						return table.id() == id;
					};

					auto table_it = std::find_if(std::begin(container_), std::end(container_), f);

					if (table_it != std::end(container_)){
						// found, move it.
						neo.push_back(std::move(*table_it));
					}else{
						// not found, add new
						neo.emplace_back();

						neo.back().open(id, filename, allocator_, mode_);
					}
				}

				container_ = std::move(neo);
			}

		private:
			Container		container_;

			CollectionList		list_{ container_ };

			DiskList::VMAllocator	*allocator_;
			DiskList::OpenMode	mode_;
		};

	} // namespace impl_
} // namespace listloader
} // namespace


#endif


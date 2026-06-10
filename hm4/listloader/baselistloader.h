#ifndef BASE_TABLE_LOADER_H_
#define BASE_TABLE_LOADER_H_

#include "disk/disklist.h"
#include "multi/collectionlist.h"
#include "myfs.h"
#include "mytime.h"

#include <string_view>

#include "smallvector.h"



namespace hm4::listloader{

	namespace impl_{

		struct ContainerHelper{
			constexpr static size_t ContainerInlineCapacity = 64;

			using DiskList		= hm4::disk::DiskList;
			using Container		= SmallVector<DiskList  , ContainerInlineCapacity>;
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
			void copy(IT first, IT last, bool sort = false){
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

				if (sort)
					return sort_(neo);

				container_ = std::move(neo);
			}

			void print() const{
				if constexpr(1){
					if (std::empty(container_)){
						logger<Logger::DEBUG>() << "Collection list is empty";
						return;
					}

					logger<Logger::DEBUG>() << "Disktables dump:";

					constexpr const char *mask = "{} {:>12}   {}   {}";

					mytime::to_string_buffer_t buffer[2];

					for(DiskList const &list : container_){
						logger_fmt<Logger::DEBUG>(mask,
								"Disk Table #",
								list.id(),
								mytime::toString(list.createdMin(), mytime::TIME_FORMAT_STANDARD, buffer[0]),
								mytime::toString(list.createdMax(), mytime::TIME_FORMAT_STANDARD, buffer[1])
						);
					}
				}
			}

		private:
			void sort_(Container &neo){
				using IContainer = SmallVector<Container::size_type, ContainerInlineCapacity>;

				IContainer ix;

				ix.reserve(neo.size());

				// create index

				for(Container::size_type i = 0; i < neo.size(); ++i)
					ix.push_back(i);

				auto f = [&neo](auto a, auto b){
					return neo[a].createdMax() > neo[b].createdMax();
				};

				// sort index

				std::sort(std::begin(ix), std::end(ix), f);

				// move

				container_.clear();
				container_.reserve(neo.size());

				for(auto &i : ix)
					container_.push_back(std::move(neo[i]));

				print();
			}

		private:
			Container		container_;

			CollectionList		list_{ container_ };

			DiskList::VMAllocator	*allocator_;
			DiskList::OpenMode	mode_;
		};

	} // namespace impl_
} // namespace hm4::listloader



#endif


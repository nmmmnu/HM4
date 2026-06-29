#ifndef ISAM_HASH_SEARCHER_H_
#define ISAM_HASH_SEARCHER_H_

#include "isam.h"
#include "hashtable/easyhashtable.h"
#include "hashtable/compactstorage.h"
#include "hashtable/tophashindex.h"
#include "hashtable/indexselector.h"

#include "staticvector.h"

namespace ISAM_impl_{

	struct FieldMapController{
		using key_type		= std::string_view;
		using mapped_type	= const Field *;
		using value_type	= const Field *;

	public:
		static size_t hash(key_type const &key){
			return std::hash<key_type>{}(key);
		}

		constexpr static key_type    const &getKey(value_type const &data){
			return data->name;
		}

		constexpr static mapped_type const &getVal(value_type const &data){
			return data;
		}
	};

	template<size_t MaxItems, size_t Size>
	using MyHashtable = myhashtable::EasyHashtable<
					FieldMapController,
					myhashtable::IndexSelector<myhashtable::TopHashIndex, FieldMapController::key_type, Size>,
					myhashtable::CompactStorage<FieldMapController::value_type, MaxItems, Size, StaticVector>
				>;



	class ISAM::HashSearcherByName{
		friend struct ISAM;

	private:
		constexpr static auto HT_ITEMS = config::CONTAINER_SIZE;
		constexpr static auto HT_CELLS = 1024;

		// HT_ITEMS * 8 + HT_CELLS
		// for 128 = 2 KB
		// for 250 = 4 KB

		MyHashtable<HT_ITEMS, HT_CELLS> ht;

	private:
		HashSearcherByName(FieldContainer const &container){
			for(auto const &field : container)
				ht.insert(&field);
		}

		const Field *operator()(std::string_view name) const{
			const Field * const *field = ht.find(name);

			return field ? *field : nullptr;
		}
	};

	inline auto ISAM::getHashSearcherByName() const -> HashSearcherByName{
		return HashSearcherByName{ container };
	}

} // namespace ISAM_impl_

#endif


#ifndef ISAM_HASH_SEARCHER_H_
#define ISAM_HASH_SEARCHER_H_

#include "isam.h"
#include "hashtable/hashtable.h"
#include "hashtable/compactstorage.h"

namespace ISAM_impl_{

	struct FieldMapController{
		using key_type		= std::string_view;
		using mapped_type	= const Field *;
		using value_type	= const Field *;

	private:
		constexpr static auto HT_ITEMS = config::CONTAINER_SIZE;
		constexpr static auto HT_CELLS = 1024;

		// HT_ITEMS * 8 + HT_CELLS
		// for 128 = 2 KB
		// for 250 = 4 KB

		using MyStorage = myhashtable::CompactStorage<value_type, HT_ITEMS, HT_CELLS>;

	public:
		constexpr static size_t size(){
			return HT_CELLS;
		}

		static size_t hash(key_type const &key){
			return std::hash<key_type>{}(key);
		}

		constexpr static key_type    const &getKey(value_type const &data){
			return data->name;
		}

		constexpr static mapped_type const &getVal(value_type const &data){
			return data;
		}

	public:
		constexpr bool operator()(size_t id) const{
			return data_(id);
		}

		constexpr value_type const &operator[](size_t id) const{
			return data_[id];
		}

		constexpr value_type &operator[](size_t id){
			return data_[id];
		}

		template<typename... Ts>
		constexpr void emplace(size_t id, Ts &&...ts){
			data_.emplace(id, std::forward<Ts>(ts)...);
		}

	public:
		constexpr bool equal(size_t id, key_type const &key) const{
			return getKey(data_[id]) == key;
		}

		void stats() const{
			return data_.stats();
		}

	private:
		MyStorage data_;
	};



	class ISAM::HashSearcherByName{
		friend struct ISAM;

		FieldMapController mapController;

		HashSearcherByName(FieldContainer const &container){
			for(auto const &field : container)
				myhashtable::insert(mapController, &field);
		}

		const Field *operator()(std::string_view name) const{
			const Field * const *field = myhashtable::find(mapController, name);

			return field ? *field : nullptr;
		}
	};

	inline auto ISAM::getHashSearcherByName() const -> HashSearcherByName{
		return HashSearcherByName{ container };
	}

} // namespace ISAM_impl_

#endif


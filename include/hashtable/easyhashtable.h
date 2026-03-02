#ifndef MY_HASHTABLE_EASY_HASHTABLE_H_
#define MY_HASHTABLE_EASY_HASHTABLE_H_

#include "hashtable.h"

namespace myhashtable{

	template<typename Controller>
	struct EasyHashtable{
		using key_type   = typename Controller::key_type;
		using value_type = typename Controller::value_type;

		template<typename... Ts>
		constexpr EasyHashtable(Ts &&...ts) : controller_(std::forward<Ts>(ts)...){}

		template<typename... Ts>
		constexpr bool insert(key_type const &key, Ts &&...ts){
			return myhashtable::insert(controller_, key, std::forward<Ts>(ts)...);
		}

		constexpr bool insert(value_type const &data){
			return myhashtable::insert(controller_, data);
		}

		constexpr bool insert(value_type &&data){
			return myhashtable::insert(controller_, std::move(data));
		}

		constexpr const typename Controller::mapped_type *find(key_type const &key) const{
			return myhashtable::find(controller_, key);
		}

		constexpr typename Controller::mapped_type *find(key_type const &key){
			return myhashtable::findMut(controller_, key);
		}

		constexpr bool exists(key_type const &key) const{
			return myhashtable::exists(controller_, key);
		}

		constexpr void stats() const{
			return controller_.stats();
		}

		constexpr size_t longestChain() const{
			return myhashtable::longestChain(controller_);
		}

	private:
		Controller controller_;
	};

} // namespace myhashtable

#endif


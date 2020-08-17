#ifndef List_DB_ADAPTER_H_
#define List_DB_ADAPTER_H_

#include <algorithm>

#include "version.h"

namespace accumulator_impl_{

	inline bool samePrefix(std::string_view const p, std::string_view const s){
		if (p.size() > s.size())
			return false;

		return std::equal(std::begin(p), std::end(p), std::begin(s));
	}

} // accumulator_impl_

template<class List, class CommandSave=std::nullptr_t, class CommandReload=std::nullptr_t>
class ListDBAdapter{
public:
	constexpr static bool MUTABLE = ! std::is_const_v<List>;

	enum class AdapterCommand : int{
		SAVE	= 1,
		RELOAD	= 2
	};

public:
	ListDBAdapter(List &list, CommandSave &cmdSave, CommandReload &cmdReload) :
				list_(list),
				cmdSave_	(& cmdSave	),
				cmdReload_	(& cmdReload	){}

//	ListDBAdapter(List &list, CommandSave &cmdSave) :
//				ListDBAdapter(list, cmdSave, cmdSave){}

public:
	// Immutable Methods

	std::string_view get(std::string_view const key) const{
		assert(!key.empty());

		return getVal_( list_.find(key, std::true_type{} ) );
	}

	template<class Accumulator>
	auto foreach(std::string_view const key, uint16_t const resultCount, std::string_view const prefix, Accumulator &accumulator) const{
		using accumulator_impl_::samePrefix;

		uint16_t count = 0;

		auto it = key.empty() ? std::begin(list_) : list_.find(key, std::false_type{} );

		for(; it != std::end(list_); ++it){
			auto const &key = it->getKey();

			if (++count > resultCount)
				return accumulator.result(key);

			if ( ! prefix.empty() && ! samePrefix(prefix, key))
				return accumulator.result();

			if (it->isValid(std::true_type{}))
				if (!accumulator(it->getKey(), it->getVal()))
					break;
		}

		return accumulator.result();
	}

public:
	std::string info() const{
		to_string_buffer_t buffer[2];

		return concatenate(
			"Version          : ", hm4::version::str,			"\n",
			"Keys (estimated) : ", to_string(list_.size(),  buffer[0]),	"\n",
			"Size             : ", to_string(list_.bytes(), buffer[1]),	"\n",
			"Mutable          : ", MUTABLE ? "Yes" : "No",			"\n"
		);
	}

	auto save(){
		return invokeCommand__(cmdSave_);
	}

	auto reload(){
		return invokeCommand__(cmdReload_);
	}

private:
	template<class Command>
	static bool invokeCommand__(Command *cmd){
		if constexpr(std::is_same_v<Command,std::nullptr_t>)
			return false;
		else
			return cmd && cmd->command();
	}

public:
	// Mutable Methods

	void set(std::string_view const key, std::string_view const val, uint32_t expires = 0){
		assert(!key.empty());

		list_.insert(key, val, expires);
	}

	bool del(std::string_view const key){
		assert(!key.empty());

		return list_.erase(key);
	}

private:
	std::string_view getVal_(typename List::iterator const &it) const{
		if (it != std::end(list_) && it->isValid(std::true_type{}))
			return it->getVal();
		else
			return {};
	}

private:
	List		&list_;
	CommandSave	*cmdSave_	= nullptr;
	CommandReload	*cmdReload_	= nullptr;
};

#endif


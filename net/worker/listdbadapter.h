#ifndef List_DB_ADAPTER_H_
#define List_DB_ADAPTER_H_

#include "version.h"
#include "myprocess.h"	// get PID

template<class List, class CommandSave=std::nullptr_t, class CommandReload=std::nullptr_t>
class ListDBAdapter{
public:
	constexpr static bool MUTABLE = ! std::is_const_v<List>;

public:
	ListDBAdapter(List &list, CommandSave &cmdSave, CommandReload &cmdReload) :
				list_(list),
				cmdSave_	(& cmdSave	),
				cmdReload_	(& cmdReload	){}

//	ListDBAdapter(List &list, CommandSave &cmdSave) :
//				ListDBAdapter(list, cmdSave, cmdSave){}

public:
	// Help Methods

	std::string_view get(std::string_view const key){
		auto it = find(key);

		return valid(it) ? it->getVal() : "";
	}

	bool valid(typename List::iterator const &it){
		return it != std::end(list_) && it->isValid(std::true_type{});
	}

public:
	// System Methods

	std::string_view info(std::string &str) const{
		to_string_buffer_t buffer[3];

		concatenate(
			str,

			"Version          : ", hm4::version::str,				"\n",
			"Keys (estimated) : ", to_string(list_.size(),		buffer[0]),	"\n",
			"Size             : ", to_string(list_.bytes(),		buffer[1]),	"\n",
			"Mutable          : ", MUTABLE ? "Yes" : "No",				"\n",
			"PID              : ", to_string(getProcessID(),	buffer[2]),	"\n"
		);

		return std::string_view{ str };
	}

	constexpr static std::string_view version(){
		return hm4::version::str;
	}

	auto save(){
		return invokeCommand__(cmdSave_);
	}

	auto reload(){
		return invokeCommand__(cmdReload_);
	}

public:
	// Immutable Methods

	template<bool B = true>
	auto find(std::string_view const key, std::bool_constant<B> tag = {}) const{
		return hm4::getIterator(list_, key, tag);
	}

	auto end() const{
		return std::end(list_);
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
	template<class Command>
	static bool invokeCommand__(Command *cmd){
		if constexpr(std::is_same_v<Command,std::nullptr_t>)
			return false;
		else
			return cmd && cmd->command();
	}

private:
	List		&list_;
	CommandSave	*cmdSave_	= nullptr;
	CommandReload	*cmdReload_	= nullptr;
};

#endif


#ifndef List_DB_ADAPTER_H_
#define List_DB_ADAPTER_H_

#include "version.h"
#include "myprocess.h"

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

	auto getAll(std::string_view const key) const{
		assert(!key.empty());

		auto it = list_.find(key, std::true_type{});

		struct R{
		//	std::string_view	key;
			std::string_view	val;
			uint32_t		exp;
		};

		return R{
			getVal_(it),
			getTTL_(it)
		};
	}

	auto search(std::string_view const key = "") const{
		return key.empty() ? std::begin(list_) : list_.find(key, std::false_type{} );
	}

	auto end() const{
		return std::end(list_);
	}

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

	uint32_t getTTL_(typename List::iterator const &it) const{
		if (it != std::end(list_) && it->isValid(std::true_type{}))
			return it->getTTL();
		else
			return 0;
	}

private:
	List		&list_;
	CommandSave	*cmdSave_	= nullptr;
	CommandReload	*cmdReload_	= nullptr;
};

#endif


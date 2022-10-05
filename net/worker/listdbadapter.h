#ifndef List_DB_ADAPTER_H_
#define List_DB_ADAPTER_H_

#include "version.h"
#include "myprocess.h"	// get PID



template<class List, class CommandSave=std::nullptr_t, class CommandReload=std::nullptr_t>
struct ListDBAdapter{
	constexpr static bool MUTABLE = ! std::is_const_v<List>;
	constexpr static bool ENABLE_UPDATE_INPLACE = true;

	constexpr static inline std::string_view SEPARATOR = "~";

	struct IteratorAdapter{
		using Iterator = typename List::iterator;

		IteratorAdapter(Iterator it, Iterator end) : it(std::move(it)), end(std::move(end)){}

		const hm4::Pair *operator->() const{
			return & *it;
		}

	//	const hm4::Pair &operator*() const{
	//		return *it;
	//	}

		void operator++(){
			++it;
		}

		operator bool() const{
			return it != end;
		}

	private:
		Iterator it;
		Iterator end;
	};

public:
	ListDBAdapter(List &list, CommandSave &cmdSave, CommandReload &cmdReload) :
				list_(list),
				cmdSave_	(& cmdSave	),
				cmdReload_	(& cmdReload	){}

//	ListDBAdapter(List &list, CommandSave &cmdSave) :
//				ListDBAdapter(list, cmdSave, cmdSave){}

public:
	// System Methods

	std::string_view info(std::string &str) const{
		to_string_buffer_t buffer[3];

		concatenateBuffer(
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
		return IteratorAdapter{
			hm4::getIterator(list_, key, tag),
			std::end(list_)
		};
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

	template<typename T>
	[[nodiscard]]
	T *canUpdateInPlace(const T *p){
		if constexpr(ENABLE_UPDATE_INPLACE){
			return canUpdateInPlace_(p);
		}else{
			return nullptr;
		}
	}

private:
	template<typename T>
	[[nodiscard]]
	T *canUpdateInPlace_(const T *p){
		if (list_.getAllocator().owns(p)){
			// pointer is in a Pair in the memlist and it is safe to be overwitten.
			// the create time is not updated, but this is not that important,
			// since the Pair is not yet flushed.

			// function is deliberately non const,
			// because the Pair will be overwritten soon anyway.
			return const_cast<T *>(p);
		}

		return nullptr;
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


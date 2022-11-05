#ifndef List_DB_ADAPTER_H_
#define List_DB_ADAPTER_H_

#include "version.h"
#include "myprocess.h"	// get PID
#include "logger.h"

template<class List, class CommandSave=std::nullptr_t, class CommandReload=std::nullptr_t>
struct ListDBAdapter{
	constexpr static bool MUTABLE 			= ! std::is_const_v<List>;
	constexpr static bool TRY_INSERT_HINTS		= true;
	constexpr static bool DELETE_USE_TOMBSTONES	= true;
	constexpr static auto LOG_LEVEL			= LogLevel::NOTICE;

	constexpr static inline std::string_view SEPARATOR = "~";

	struct IteratorAdapter{
		using Iterator = typename List::iterator;

		IteratorAdapter(Iterator it, Iterator end) : it(std::move(it)), end(std::move(end)){}

		const hm4::Pair *operator->() const{
			return & *it;
		}

		const hm4::Pair &operator*() const{
			return *it;
		}

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

	template<size_t N>
	std::string_view info(std::array<char, N> &str) const{
		to_string_buffer_t buffer[6];

		if constexpr(!MUTABLE){
			return concatenateBuffer(
				str,

				"# Server"											"\n",
				"Version          : ", hm4::version::str				,			"\n",

			"\n"	"# Keys"											"\n",
				"Mutable          : ", MUTABLE ? "Yes" : "No"				,			"\n",
				"Keys (estimated) : ", to_string(list_.size()				,	buffer[0]),	"\n",
				"Size             : ", to_string(list_.bytes()				,	buffer[1]),	"\n",

			"\n"	"# System"											"\n",
				"PID              : ", to_string(getProcessID()				,	buffer[4]),	"\n"
			);
		}else{
			auto mem_format = [](uint64_t const a, to_string_buffer_t &buffer) -> std::string_view{
				if (a == std::numeric_limits<std::size_t>::max())
					return "n/a";
				else
					return to_string(a, buffer);
			};

			return concatenateBuffer(
				str,

				"# Server"											"\n",
				"Version          : ", hm4::version::str				,			"\n",

			"\n"	"# Keys"											"\n",
				"Mutable          : ", MUTABLE ? "Yes" : "No"				,			"\n",
				"Keys (estimated) : ", to_string(list_.size()				,	buffer[0]),	"\n",
				"Size             : ", to_string(list_.bytes()				,	buffer[1]),	"\n",
				"Mutable Keys     : ", to_string(list_.mutable_size()			,	buffer[2]),	"\n",

			"\n"	"# Allocator"											"\n",
				"Allocator        : ", list_.getAllocator().getName()			,			"\n",
				"Allocator Free   : ", mem_format(list_.getAllocator().getFreeMemory()	,	buffer[3]),	"\n",
				"Allocator Used   : ", mem_format(list_.getAllocator().getUsedMemory()	,	buffer[4]),	"\n",

			"\n"	"# System"											"\n",
				"PID              : ", to_string(getProcessID()				,	buffer[5]),	"\n"
			);
		}
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

		insert(list_, key, val, expires);
	}

	void setHint(const hm4::Pair *pair, std::string_view const val, uint32_t expires = 0){
		if constexpr(TRY_INSERT_HINTS){
			if (hm4::tryInsertHint(list_, pair, pair->getKey(), val, expires))
				return logHint__("setHint");
		}

		return set(pair->getKey(), val, expires);
	}

	void expHint(const hm4::Pair *pair, uint32_t expires){
		if constexpr(TRY_INSERT_HINTS){
			if (hm4::tryInsertHint(list_, pair, expires, pair->getKey(), pair->getVal()))
				return logHint__("expHint");
		}

		return set(pair->getKey(), pair->getVal(), expires);
	}

	bool del(std::string_view const key){
		assert(!key.empty());

		erase(list_, key);

		return true;
	}

	bool delHint(const hm4::Pair *pair){
		if constexpr(TRY_INSERT_HINTS && DELETE_USE_TOMBSTONES){
			// this is a bit ugly,
			// because ListDBAdapter not suppose to know,
			// if it is with tombstone or not.
			if (hm4::tryInsertHint(list_, pair, pair->getKey()))
				return logHintBool__("delHint");
		}

		return del(pair->getKey());
	}

	auto mutable_size() const{
		return list_.mutable_size();
	}

	bool canUpdateWithHint(const hm4::Pair *p) const{
		return list_.getAllocator().owns(p);
	}

private:
	static void logHint__(const char *msg){
		log__<LOG_LEVEL>(msg, "Bypassing list");
	}

	static bool logHintBool__(const char *msg){
		logHint__(msg);
		return true;
	}

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


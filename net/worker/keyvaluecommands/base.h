#ifndef _KEY_VALUE_COMMANDS_BASE_H
#define _KEY_VALUE_COMMANDS_BASE_H

#include "myspan.h"
#include "iobuffer.h"
#include "../workerdefs.h"

#include "staticvector.h"
#include "mystring.h"

//#include <variant>

namespace net::worker::commands{



	enum class Status{
		OK,

		DISCONNECT,
		SHUTDOWN
	};



	struct OutputBlob{
		constexpr static size_t ContainerSize	= 65'536;
		using Container = StaticVector<std::string_view,ContainerSize>;

		constexpr static size_t BufferKeySize	= hm4::PairConf::MAX_KEY_SIZE + 16;
		using BufferKey = std::array<char, BufferKeySize>;

		OutputBlob(){
			container.reserve(ContainerSize);
		}

		Container	container;
		BufferKey	buffer_key;
	//	std::string	string;
	};



	template<class Protocol>
	class Result{
		Protocol	&protocol_;
		IOBuffer	&buffer_;

		Status          status  = Status::OK;

	public:
		using Container = MySpan<std::string_view, MySpanConstructor::EXPLICIT>;

		Result(Protocol &protocol, IOBuffer &buffer) :
						protocol_	(protocol	),
						buffer_		(buffer		){}

		auto getStatus() const{
			return status;
		}

	public:
		void set_status(Status status2){
			status = status2;
		}

	//	void error(){
	//		return set_status(Status::ERROR);
	//	}

		void set(){
			set_status(Status::OK);

			buffer_.clear();
			protocol_.response_ok(buffer_);
		}

		void set(bool b){
			set_status(Status::OK);

			buffer_.clear();
			protocol_.response_bool(buffer_, b);
		}

		void set(std::string_view s){
			set_status(Status::OK);

			buffer_.clear();
			protocol_.response_string(buffer_, s);
		}

		void set_container(MySpan<std::string_view> const &container){
			set_status(Status::OK);

			buffer_.clear();
			protocol_.response_strings(buffer_, container);
		}

		void set_0(){
			return set("0");
		}

		void set_1(){
			return set("1");
		}

		void set(int64_t number){
			return set_number(number);
		}

		void set(uint64_t number){
			return set_number(number);
		}

	private:
		template<typename T>
		void set_number(T number){
			to_string_buffer_t std_buffer;

			std::string_view const val = to_string(number, std_buffer);

			return set(val);
		}
	};



	using ParamContainer  = MySpan<std::string_view>;



	template<class Protocol, class DBAdapter>
	struct Base{
		constexpr static bool mut = false;

		virtual ~Base() = default;

		void operator()(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			return process(params, db, result, blob);
		}

	private:
		virtual void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) = 0;
	};



	template<
		template<class, class>  class Cmd,
		class Protocol,
		class DBAdapter,
		class RegisterPack
	>
	void registerCommand(RegisterPack &pack){
		using Command		= Cmd <Protocol, DBAdapter>;
		using CommandBase	= Base<Protocol, DBAdapter>;

		static_assert(std::is_base_of_v<CommandBase, Command>, "Command not seems to be a command");

		if constexpr(Command::mut == false || Command::mut == DBAdapter::MUTABLE ){
			auto &up = pack.storage.emplace_back(std::make_unique<Command>());

			CommandBase *p = up.get();

			for(auto const &key : Command::cmd)
				pack.map.emplace(key, p);
		}
	}



	template<
		class Protocol,
		class DBAdapter,
		class RegisterPack,
		template<class,class> typename... Commands
	>
	void registerCommands(RegisterPack &pack){
		( registerCommand<Commands, Protocol, DBAdapter>(pack), ... );
	}



}


#endif


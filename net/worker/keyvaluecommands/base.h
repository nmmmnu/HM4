#ifndef _KEY_VALUE_COMMANDS_BASE_H
#define _KEY_VALUE_COMMANDS_BASE_H

#include "myspan.h"
#include "iobuffer.h"
#include "../workerdefs.h"

#include "staticvector.h"
#include "mystring.h"

#include "logger.h"

#include <memory>

namespace net::worker::commands{



	enum class Status{
		OK,

		DISCONNECT,
		SHUTDOWN
	};



	struct OutputBlob{
		constexpr static size_t ContainerSize		= 0xFFFF;
		constexpr static size_t ParamContainerSize	= 0xFF;

		using Container		= StaticVector<std::string_view		, ContainerSize>;	// 1024 KB, if string_view is 16 bytes
		using PairContainer	= StaticVector<const hm4::Pair *	, ContainerSize>;	//  514 KB
		using BufferContainer	= StaticVector<to_string_buffer_t	, ContainerSize>;	// 2048 KB, if to_string_buffer_t is 32 bytes

		constexpr static void resetAllocator(){
		}

		auto &container(){
			return getClean__(pack->container);
		}

		auto &pcontainer(){
			return getClean__(pack->pcontainer);
		}

		auto &bcontainer(){
			return getClean__(pack->bcontainer);
		}

	private:
		template<class T>
		static T &getClean__(T &container){
			container.clear();
			return container;
		}

	private:
		struct Pack{
			Container	container;
			PairContainer	pcontainer;
			BufferContainer	bcontainer;
		};

		std::unique_ptr<Pack> pack = std::make_unique<Pack>();
	};



	template<class Protocol>
	class Result{
		Protocol	&protocol_;
		IOBuffer	&buffer_;

		Status          status  = Status::OK;

	public:
		using Container = MySpan<const std::string_view, MySpanConstructor::EXPLICIT>;

		Result(Protocol &protocol, IOBuffer &buffer) :
						protocol_	(protocol	),
						buffer_		(buffer		){}

		auto getStatus() const{
			return status;
		}

	private:
		void set_status_(Status status2){
			status = status2;
		}

	public:
	//	void error(){
	//		return set_status(Status::ERROR);
	//	}

		void set_system(Status status){
			set_status_(status);

			buffer_.clear();
			protocol_.response_ok(buffer_);
		}

		void set_error(std::string_view errorMessage){
			assert(!errorMessage.empty());

			set_status_(Status::OK);

			buffer_.clear();
			protocol_.response_error(buffer_, errorMessage);
		}

		void set(){
			set_status_(Status::OK);

			buffer_.clear();
			protocol_.response_ok(buffer_);
		}

		void set(bool b){
			set_status_(Status::OK);

			buffer_.clear();
			protocol_.response_bool(buffer_, b);
		}

		void set(std::string_view s){
			set_status_(Status::OK);

			buffer_.clear();
			protocol_.response_string(buffer_, s);
		}

		// avoid convert const char * to bool
		void set(const char *s){
			return set(
				std::string_view(s)
			);
		}

		void set_simple_string(std::string_view s){
			set_status_(Status::OK);

			buffer_.clear();
			protocol_.response_simple_string(buffer_, s);
		}

		void set_container(MySpan<const std::string_view> const &container){
			set_status_(Status::OK);

			buffer_.clear();
			protocol_.response_strings(buffer_, container);
		}

		void set_dual(std::string_view s0, std::string_view s1){
			std::array<std::string_view, 2> const container{ s0, s1 };

			set_status_(Status::OK);

			buffer_.clear();
			protocol_.response_strings(buffer_, container);
		}

		void set_0(){
			return set_number_sv("0");
		}

		void set_1(){
			return set_number_sv("1");
		}

		void set(int64_t number){
			return set_number(number);
		}

		void set(uint64_t number){
			return set_number(number);
		}

		#ifdef SIZE_T_SEPARATE_FROM_UINT64_T
		void set(size_t number){
			return set_number<uint64_t>(number);
		}
		#endif

		void set_number_sv(std::string_view number){
			set_status_(Status::OK);

			buffer_.clear();
			protocol_.response_number_sv(buffer_, number);
		}

	private:
		template<typename T>
		void set_number(T number){
			set_status_(Status::OK);

			buffer_.clear();
			protocol_.response_number(buffer_, number);
		}
	};



	using ParamContainer  = MySpan<const std::string_view>;



	template<class Protocol, class DBAdapter>
	struct BaseCmd{
		bool mut() const{
			return mut_();
		}

		virtual const std::string_view *begin() const = 0;
		virtual const std::string_view *end()   const = 0;

		virtual ~BaseCmd() = default;

		void operator()(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			return process(params, db, result, blob);
		}

	private:
		virtual void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) = 0;
		virtual bool mut_() const = 0;
	};



	template<class Protocol, class DBAdapter, bool Mutable>
	struct BaseCmd_ : BaseCmd<Protocol,DBAdapter>{
		constexpr static bool MUTABLE = Mutable;

	private:
		virtual bool mut_() const final{
			return MUTABLE;
		}
	};

	template<class Protocol, class DBAdapter>
	using BaseCmdRO = BaseCmd_<Protocol,DBAdapter,false>;

	template<class Protocol, class DBAdapter>
	using BaseCmdRW = BaseCmd_<Protocol,DBAdapter,true>;



	template<
		template<class, class>  class Cmd,
		class Protocol,
		class DBAdapter,
		class RegisterPack
	>
	void registerCommand(RegisterPack &pack){
		using Command		= Cmd		<Protocol, DBAdapter>;
		using CommandBase	= BaseCmd	<Protocol, DBAdapter>;
		using CommandBaseRO	= BaseCmdRO	<Protocol, DBAdapter>;
		using CommandBaseRW	= BaseCmdRW	<Protocol, DBAdapter>;

		static_assert(	std::is_base_of_v<CommandBase, Command>,
							"Command not seems to be a command");

		static_assert(	std::is_base_of_v<CommandBaseRO, Command> ||
				std::is_base_of_v<CommandBaseRW, Command>,
							"Command not seems to be a command");

		bool const mut = std::is_base_of_v<CommandBaseRW, Command>;

		if constexpr(mut == false || mut == DBAdapter::MUTABLE){
			auto &up = pack.storage.emplace_back(std::make_unique<Command>());

			for(auto const &key : *up)
				pack.map.emplace(key, up.get());
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


#if 0
	template<class Protocol>
	struct BasePipe{
		virtual ~BasePipe() = default;

		virtual const std::string_view *begin() const = 0;
		virtual const std::string_view *end()   const = 0;

		virtual bool init(ParamContainer const &params, OutputBlob &blob) = 0;

		virtual void done(Result<Protocol> &result) = 0;

		virtual bool process(const hm4::Pair *) = 0;
	};

	template<class Protocol>
	struct DonePipe : BasePipe<Protocol>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		virtual bool init(ParamContainer const &, OutputBlob &){
			return true;
		}

		virtual void done(Result<Protocol> &result){
			return result.set();
		}

		virtual bool process(const hm4::Pair *){
			return false;
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"done",	"DONE"
		};
	};
#endif

}


#endif


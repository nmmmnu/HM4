#ifndef _KEY_VALUE_COMMANDS_BASE_H
#define _KEY_VALUE_COMMANDS_BASE_H

#include "myspan.h"
#include "iobuffer.h"
#include "../workerdefs.h"

#include "staticvector.h"
#include "mystring.h"

#include "logger.h"

#include <memory>
#include <vector>
#include <unordered_map>

namespace net::worker::commands{



	enum class Status{
		OK,

		DISCONNECT,
		SHUTDOWN
	};



	struct OutputBlob{
		constexpr static size_t ContainerSize		= 0xFFFF;
		constexpr static size_t ParamContainerSize	= 0xFF;

		using buffer_t = std::array<char, 128>;

		using Container		= StaticVector<std::string_view		, ContainerSize>;	// 1024 KB, if string_view is 16 bytes
		using PairContainer	= StaticVector<const hm4::Pair *	, ContainerSize>;	//  514 KB
		using BufferContainer	= StaticVector<buffer_t			, ContainerSize>;	// 8191 KB

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
	struct BaseCommand{
		bool mut() const{
			return mut_();
		}

		virtual const std::string_view *begin() const = 0;
		virtual const std::string_view *end()   const = 0;

		virtual ~BaseCommand() = default;

		void operator()(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			return process(params, db, result, blob);
		}

	private:
		virtual void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) = 0;
		virtual bool mut_() const = 0;
	};



	template<class Protocol, class DBAdapter, bool Mutable>
	struct BaseCommand_ : BaseCommand<Protocol,DBAdapter>{
		constexpr static bool MUTABLE = Mutable;

	private:
		virtual bool mut_() const final{
			return MUTABLE;
		}
	};

	template<class Protocol, class DBAdapter>
	using BaseCommandRO = BaseCommand_<Protocol,DBAdapter,false>;

	template<class Protocol, class DBAdapter>
	using BaseCommandRW = BaseCommand_<Protocol,DBAdapter,true>;



	template<class Protocol, class DBAdapter>
	struct StorageCommands{
		using BaseObject	= BaseCommand<Protocol, DBAdapter>;
		using BasePtr		= std::unique_ptr<BaseObject>;
		using Storage		= std::vector<BasePtr>;
		using Map		= std::unordered_map<std::string_view, BaseObject *>;

		constexpr static bool RegisterDebugPrint = true;

		BaseObject *operator()(std::string_view key){
			auto it = map_.find(key);

			if (it == std::end(map_))
				return nullptr;

			auto &command = *it->second;

			return & command;
		}

		template<typename Object>
		void push(){
			storage_.push_back( std::make_unique<Object>() );

			auto &x = storage_.back();

			debugPrint(*x);

			for(auto const &key : *x)
				map_.emplace(key, x.get());
		}

	private:
		static void debugPrint(BaseObject const &x){
			if constexpr(!RegisterDebugPrint)
				return;

			if (auto it = std::begin(x); it != std::end(x))
				logger<Logger::DEBUG>() << " - " << *it;
		}

	private:
		Storage	storage_	;
		Map	map_		;
	};



	namespace registration_impl_{

		template<
			template<class, class>  class Command,
			class Protocol,
			class DBAdapter,
			class RegisterPack
		>
		void registerCommand(RegisterPack &pack){
			StorageCommands<Protocol, DBAdapter> &storage = pack.storageCommands;


			using MyCommand		= Command	<Protocol, DBAdapter>;
			using MyCommandBase	= BaseCommand	<Protocol, DBAdapter>;
			using MyCommandBaseRO	= BaseCommandRO	<Protocol, DBAdapter>;
			using MyCommandBaseRW	= BaseCommandRW	<Protocol, DBAdapter>;

			static_assert(	std::is_base_of_v<MyCommandBase, MyCommand>,
								"Command not seems to be a command");

			static_assert(	std::is_base_of_v<MyCommandBaseRO, MyCommand> ||
					std::is_base_of_v<MyCommandBaseRW, MyCommand>,
								"Command not seems to be a command");

			bool const mut = std::is_base_of_v<MyCommandBaseRW, MyCommand>;

			if constexpr(mut == false || mut == DBAdapter::MUTABLE)
				storage. template push<MyCommand>();
		}

	} // namespace registration_impl_



	template<
		class Protocol,
		class DBAdapter,
		class RegisterPack,
		template<class,class> typename... Commands
	>
	void registerCommands(RegisterPack &pack){
		using namespace registration_impl_;

		( registerCommand<Commands, Protocol, DBAdapter>(pack), ... );
	}
}


#endif


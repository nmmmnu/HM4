#ifndef _KEY_VALUE_COMMANDS_BASE_H
#define _KEY_VALUE_COMMANDS_BASE_H

#define USE_GPERF

#include "myspan.h"
#include "iobuffer.h"
#include "../workerdefs.h"

#include "staticvector.h"
#include "mystring.h"

#include "logger.h"

#include <memory>

#ifndef USE_GPERF
	#include "hashtable/easymap.h"
	#include "hashtable/compactstorage.h"
#else
	#include "gperf.h"
#endif

#include "mmapbuffer.h"
#include "arenaallocator.h"



namespace net::worker::commands{

	enum class Status{
		OK,

		DISCONNECT,
		SHUTDOWN
	};

	constexpr size_t HashtableSize		= 4096;
	constexpr size_t MaxCommands		=  350;
	constexpr size_t MaxCommandsAliases	= 1024;

	struct OutputBlob{
		constexpr static size_t ContainerSize			= 0xFFFF;
		constexpr static size_t ParamContainerSize		= 0xFF;
		constexpr static size_t ParamContainerSizeHTSize	= 1024;

		static_assert(OutputBlob::ContainerSize > OutputBlob::ParamContainerSize);

		// KeyValue worker checks the size against Protocol::MAX_PARAMS

		using buffer_t = std::array<char, 128>;

		using Container		= StaticVector<std::string_view		, ContainerSize>;	//  1   MB, if string_view is 16 bytes
		using BufferContainer	= StaticVector<buffer_t			, ContainerSize>;	//  8   MB
		using PairContainer	= StaticVector<const hm4::Pair *	, ContainerSize>;	//  0.5 MB

		using BufferKContainer	= StaticVector	<hm4::PairBufferKey	, ContainerSize>;	// ~64  MB

		constexpr static size_t MaxMemory =	sizeof(Container	)	+	//   1   MB
							sizeof(BufferContainer	)	+	//   8   MB
							sizeof(PairContainer	)	+	//   0.5 MB
							hm4::Pair::maxBytes()		+	// 256   MB !!!, bit bigger than hm4::PairBufferVal
							64
		;

		void resetAllocator(){
			allocations_ = 0;
			allocator_.reset();
		}

		template<typename T>
		auto &allocate(){
			if (auto *p = MyAllocator::allocate<T>(allocator_); p){
				logAllocatorStatus_<Logger::DEBUG, T>(false);

				++allocations_;

				return *p;
			}else{
				logAllocatorStatus_<Logger::FATAL, T>(false, "PLEASE REPORT THIS:");

				++allocations_;

				throw std::bad_alloc();
			}
		}

		template<typename T, typename ...Args>
		auto &construct(Args &&...args){
			if (auto *p = MyAllocator::construct<T>(allocator_, std::forward<Args>(args)...); p){
				logAllocatorStatus_<Logger::DEBUG, T>(true);

				++allocations_;

				return *p;
			}else{
				logAllocatorStatus_<Logger::FATAL, T>(true, "PLEASE REPORT THIS:");

				++allocations_;

				throw std::bad_alloc();
			}
		}

		template<class T>
		void destruct(T *p){
			MyAllocator::destruct<T>(allocator_, p);
		}

	private:
		template<Logger::Level level, typename T>
		void logAllocatorStatus_(bool construct, std::string_view title = ""){
			if (!title.empty())
				logger<level>() << title;

			auto const op = construct ? "Construct" : "Allocate";

			logger<level>() << "Allocation" << allocations_
					<< op
					<< "Type" << typeid(T).name()
					<< "Size" << sizeof(T) << "bytes."
					<< "Free" << allocator_.getFreeMemory() << "bytes."
					<< "Used" << allocator_.getUsedMemory() << "bytes.";
		}

	private:
		MyBuffer::MMapMemoryResource			buffer_		{ MaxMemory };
	//	MyBuffer::AllocatedMemoryResourceOwned<>	buffer_		{ MaxMemory };
		MyAllocator::ArenaAllocator			allocator_	{ buffer_   };

		size_t						allocations_	= 0;
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



	using ParamContainer		= MySpan<const std::string_view>;
	using CommandAliasesContainer	= std::array<std::string_view, 32>;



	template<class Protocol, class DBAdapter>
	struct BaseCommand{
		BaseCommand(std::string_view name, const std::string_view *cmd_begin, const std::string_view *cmd_end, bool mut) :
							name_		{ name		},
							cmd_begin_	{ cmd_begin	},
							cmd_end_	{ cmd_end	},
							mut_		{ mut		}{

			assert(std::distance(cmd_begin_, cmd_end_) > 0);
		}

		virtual ~BaseCommand() = default;

		void operator()(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			return process(params, db, result, blob);
		}

		constexpr bool mut() const{
			return mut_;
		}

		constexpr auto getName() const{
			return name_;
		}

		constexpr auto begin() const{
			return cmd_begin_;
		}

		constexpr auto end() const{
			return cmd_end_;
		}

	private:
		std::string_view	name_;
		const std::string_view	*cmd_begin_;
		const std::string_view	*cmd_end_;
		bool			mut_;

	private:
		virtual void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) = 0;
	};



	template<class Protocol, class DBAdapter, bool B>
	struct BaseCommandMM : BaseCommand<Protocol,DBAdapter>{
		BaseCommandMM(std::string_view name, const std::string_view *cmd_begin, const std::string_view *cmd_end) :
					BaseCommand<Protocol,DBAdapter>(name, cmd_begin, cmd_end, B){}
	};

	template<class Protocol, class DBAdapter>
	using BaseCommandRO = BaseCommandMM<Protocol,DBAdapter, 0>;

	template<class Protocol, class DBAdapter>
	using BaseCommandRW = BaseCommandMM<Protocol,DBAdapter, 1>;



	template<class Protocol, class DBAdapter>
	struct StorageCommands{
		using BaseObject	= BaseCommand<Protocol, DBAdapter>;
		using BasePtr		= std::unique_ptr<BaseObject>;
		using Storage		= StaticVector<BasePtr, MaxCommands>;

		constexpr static bool RegisterDebugPrint = false;

		#ifndef USE_GPERF

		template<typename T, size_t MaxItems, size_t Size>
		using MyStorage	= myhashtable::CompactStorage<T, MaxItems, Size, StaticVector>;

		using Map		= myhashtable::EasyMap<std::string_view, BaseObject *, MaxCommandsAliases, HashtableSize>;

		BaseObject *operator()(std::string_view key){
			BaseObject **command = map_.find(key);

			if (!command)
				return nullptr;

			return *command;
		}

		#else

		BaseObject *operator()(std::string_view key){
			const auto *row = gperf::CommandPerfectHash::in_word_set(key.data(), key.size());

			if (!row)
				return nullptr;

			return static_cast<BaseObject *>(row->ptr);
		}

		#endif

		template<typename Object>
		void push(){
			storage_.push_back( std::make_unique<Object>() );

			auto &x = storage_.back();

			debugPrint(*x);

			#ifndef USE_GPERF

				for(auto const &key : *x){
					if (map_.find(key)){
						logger<Logger::FATAL>() << "Duplicate command" << key << "Please report a bug!";
						assert(false);
					}

					[[maybe_unused]]
					auto const b = map_.insert(key, x.get());

					assert(b);

					++CommandAliases_;
				}

			#else

				// fill perfect hashtable with pointers
				for(auto itk = std::begin(*x); itk != std::end(*x); ++itk){
					size_t found = 0;

					for(auto &row : gperf::wordlist){
						if (	row.name  && row.group	&&
							row.name  == *itk	&&
							row.group == x->getName()
								){

							++found;
							row.ptr	= x.get();
						}
					}

					++CommandAliases_;

					if (found != 1){
						logger<Logger::FATAL>() << "Command" << x->getName() << "::" << *itk << "not found in perfect hashtable. Please report a bug!";
						assert(false);
					}
				}

			#endif
		}

		void infoPrint() const{
			logger<Logger::NOTICE>() << "Total commands" << storage_.size();
			logger<Logger::NOTICE>() << "Total CommandAliases " << CommandAliases_;

			#ifndef USE_GPERF

				if (auto const chain = map_.longestChain(); chain >= 16)
					logger<Logger::WARNING>() << "Hashtable longest chain " << map_.longestChain() << "(chain too long)";
				else
					logger<Logger::NOTICE>()  << "Hashtable longest chain " << map_.longestChain() << "(good)";

			#else

				auto const size = sizeof(gperf::wordlist) / sizeof(gperf::wordlist[0]);

				logger<Logger::NOTICE>()  << "Using perfect hashtable with size " << size;

			#endif
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

		#ifndef USE_GPERF
		Map	map_		;
		#endif

		size_t	CommandAliases_	= 0;
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


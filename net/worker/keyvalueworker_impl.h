
#include "protocol/protocoldefs.h"

#include "keyvaluecommands.h"

#include <algorithm>
#include <type_traits>
#include <sstream>

namespace net{
namespace worker{

template<class PROTOCOL, class DB_ADAPTER, class CONNECTION>
class KeyValueWorkerProcessor{
public:
	using Command = RedisCommands::Command;

public:
	KeyValueWorkerProcessor(PROTOCOL &protocol, DB_ADAPTER &db, CONNECTION &buffer) :
					protocol_(protocol),
					db_(db),
					buffer_(buffer){}

public:
	WorkerStatus operator()(){
		using Status  = protocol::ProtocolStatus;

		// PASS

		if (buffer_.size() == 0)
			return WorkerStatus::PASS;

		const Status status = protocol_( StringRef{ buffer_.data(), buffer_.size() } );

		if (status == Status::BUFFER_NOT_READ)
			return WorkerStatus::PASS;

		// ERROR

		buffer_.clear();

		if (status == Status::ERROR)
			return err_InternalError_();

		// fetch command

		const auto &cmd = protocol_.getParams().front();

		const auto &commands = RedisCommands::commands;

		const auto &it = std::find(commands.begin(), commands.end(), cmd);

		// ERROR

		if (it == commands.end())
			return err_NotImplemented_();

		// EXEC
		return executeCommand_(it->cmd);
	}

private:
	template<bool T>
	struct mutable_tag{};

	WorkerStatus executeCommand_(const Command cmd){
		using mutable_tag_c = mutable_tag<DB_ADAPTER::IS_MUTABLE>;

		switch(cmd){
		case Command::EXIT	: return WorkerStatus::DISCONNECT;
		case Command::SHUTDOWN	: return WorkerStatus::SHUTDOWN;

		case Command::REFRESH	: return do_refresh();
		case Command::INFO	: return do_info();
		case Command::GET	: return do_get();
		case Command::GETALL	: return do_getall();

		case Command::SET	: return do_set  (mutable_tag_c{});
		case Command::SETEX	: return do_setex(mutable_tag_c{});
		case Command::DEL	: return do_del  (mutable_tag_c{});

		default			: return err_NotImplemented_();
		}
	}

private:
	WorkerStatus err_BadRequest_(){
		protocol_.response_error(buffer_, "Bad request");
		return WorkerStatus::WRITE;
	}

	WorkerStatus err_NotImplemented_(){
		protocol_.response_error(buffer_, "Not Implemented");
		return WorkerStatus::WRITE;
	}

	WorkerStatus err_InternalError_(){
		protocol_.response_error(buffer_, "Internal Error");
		return WorkerStatus::WRITE;
	}

private:
	WorkerStatus do_refresh(){
		db_.refresh();

		protocol_.response_ok(buffer_);

		return WorkerStatus::WRITE;
	}

	WorkerStatus do_info(){
		const auto &p = protocol_.getParams();

		if (p.size() != 1)
			return err_BadRequest_();

		protocol_.response_string(buffer_, db_.info());

		return WorkerStatus::WRITE;
	}

	WorkerStatus do_get(){
		const auto &p = protocol_.getParams();

		if (p.size() != 2)
			return err_BadRequest_();

		const auto &key = p[1];

		if (key.empty())
			return err_BadRequest_();

		protocol_.response_string(buffer_, db_.get(key));

		return WorkerStatus::WRITE;
	}

	WorkerStatus do_getall(){
		const auto &p = protocol_.getParams();

		if (p.size() != 2 && p.size() != 3 && p.size() != 4)
			return err_BadRequest_();

		const auto &key = p[1];

		switch( p.size() ){
		case 2:
			// classic case:
			// HGETALL u:

			{
				protocol_.response_strings(buffer_, db_.getall(key) );
				break;
			}

		case 3:
			// count case
			// HGETALL u: 100

			{
				const auto &count = p[2];

				protocol_.response_strings(buffer_, db_.getall(key, count) );

				break;
			}

		case 4:
			// count + prefix case
			// HGETALL u: 100 1

			{
				const auto &count  = p[2];
				const auto &prefix = p[3];

				protocol_.response_strings(buffer_, db_.getall(key, count, prefix) );

				break;
			}
		}

		return WorkerStatus::WRITE;
	}

private:
	using mutable_tag_true = mutable_tag<true>;

	WorkerStatus do_set(mutable_tag_true){
		const auto &p = protocol_.getParams();

		if (p.size() != 3)
			return err_BadRequest_();

		const auto &key = p[1];
		const auto &val = p[2];

		if (key.empty())
			return err_BadRequest_();

		db_.set(key, val);
		protocol_.response_ok(buffer_);

		return WorkerStatus::WRITE;
	}

	WorkerStatus do_setex(mutable_tag_true){
		const auto &p = protocol_.getParams();

		if (p.size() != 4)
			return err_BadRequest_();

		const auto &key = p[1];
		const auto &exp = p[2];
		const auto &val = p[3];

		if (key.empty())
			return err_BadRequest_();

		db_.set(key, val, exp);
		protocol_.response_ok(buffer_);

		return WorkerStatus::WRITE;
	}

	WorkerStatus do_del(mutable_tag_true){
		const auto &p = protocol_.getParams();

		if (p.size() != 2)
			return err_BadRequest_();

		const auto &key = p[1];

		if (key.empty())
			return err_BadRequest_();

		protocol_.response_bool(buffer_, db_.del(key));

		return WorkerStatus::WRITE;
	}

private:
	using mutable_tag_false = mutable_tag<false>;

	WorkerStatus do_set(mutable_tag_false){
		return err_NotImplemented_();
	}

	WorkerStatus do_setex(mutable_tag_false){
		return err_NotImplemented_();
	}

	WorkerStatus do_del(mutable_tag_false){
		return err_NotImplemented_();
	}

private:
	PROTOCOL	&protocol_;
	DB_ADAPTER	&db_;
	CONNECTION	&buffer_;
};


// ====================================


template<class PROTOCOL, class DB_ADAPTER>
template<class CONNECTION>
WorkerStatus KeyValueWorker<PROTOCOL, DB_ADAPTER>::operator()(CONNECTION &buffer){
	using MyKeyValueWorkerProcessor = KeyValueWorkerProcessor<PROTOCOL, DB_ADAPTER, CONNECTION>;

	MyKeyValueWorkerProcessor processor{ protocol_, db_, buffer };

	return processor();
}


} // namespace worker
} // namespace


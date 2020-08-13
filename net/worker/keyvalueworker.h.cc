
#include "protocol/protocoldefs.h"

#include "keyvaluecommands.h"

#include "mystring.h"
#include "myinvokeclassmember.h"

#include <type_traits>

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

		const Status status = protocol_( std::string_view{ buffer_.data(), buffer_.size() } );

		if (status == Status::BUFFER_NOT_READ)
			return WorkerStatus::PASS;

		// ERROR

		buffer_.clear();

		if (status == Status::ERROR)
			return err_InternalError_();

		// fetch command

		const auto &str = protocol_.getParams().front();

		const auto &cmd = RedisCommands::get(str);

		// EXEC
		return executeCommand_(cmd);
	}

private:
	WorkerStatus executeCommand_(const Command cmd){
		using mutable_type = std::integral_constant<bool, DB_ADAPTER::MUTABLE>;

		return executeCommand_(cmd, mutable_type{});
	}

	WorkerStatus executeCommand_(const Command cmd, std::false_type){
		switch(cmd){

		case Command::EXIT	: return WorkerStatus::DISCONNECT;
		case Command::SHUTDOWN	: return WorkerStatus::SHUTDOWN;

		case Command::INFO	: return do_info();
		case Command::SAVE	: return do_save();
		case Command::RELOAD	: return do_reload();

		case Command::GET	: return do_get();
		case Command::GETX	: return do_getx();

		case Command::COUNT	: return do_count();
		case Command::SUM	: return do_sum();
		case Command::MIN	: return do_min();
		case Command::MAX	: return do_max();

		default			: return err_NotImplemented_();
		}
	}

	WorkerStatus executeCommand_(const Command cmd, std::true_type){
		switch(cmd){

		case Command::SET	: return do_set();
		case Command::SETEX	: return do_setex();
		case Command::DEL	: return do_del();

		case Command::INCR	: return do_incr();
		case Command::DECR	: return do_decr();
		case Command::GETSET	: return do_getset();

		default			: return executeCommand_(cmd, std::false_type{});
		}
	}

private:
	WorkerStatus err_String_(std::string_view const error){
		protocol_.response_error(buffer_, error);
		return WorkerStatus::WRITE;
	}

	WorkerStatus err_BadRequest_(){
		return err_String_("Bad request");
	}

	WorkerStatus err_NotImplemented_(){
		return err_String_("Not Implemented");
	}

	WorkerStatus err_InternalError_(){
		return err_String_("Internal Error");
	}

private:
	WorkerStatus do_info(){
		const auto &p = protocol_.getParams();

		if (p.size() != 1)
			return err_BadRequest_();

		protocol_.response_string(buffer_, db_.info());

		return WorkerStatus::WRITE;
	}

	template<typename F>
	WorkerStatus do_save_(F func){
		const auto &p = protocol_.getParams();

		if (p.size() != 1)
			return err_BadRequest_();

		invoke_class_member(db_, func);

		protocol_.response_ok(buffer_);

		return WorkerStatus::WRITE;
	}

	WorkerStatus do_save(){
		return do_save_(&DB_ADAPTER::save);
	}

	WorkerStatus do_reload(){
		return do_save_(&DB_ADAPTER::reload);
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

	template<typename F>
	WorkerStatus do_accumulate_(F func){
		const auto &p = protocol_.getParams();

		if (p.size() != 4)
			return err_BadRequest_();

		const auto &key    = p[1];
		uint16_t const count = from_string<uint16_t>(p[2]);
		const auto &prefix = p[3];

		protocol_.response_strings(buffer_, invoke_class_member(db_, func, key, count, prefix) );

		return WorkerStatus::WRITE;
	}

	auto do_getx(){
		return do_accumulate_(&DB_ADAPTER::getx);
	}

	auto do_count(){
		return do_accumulate_(&DB_ADAPTER::count);
	}

	auto do_sum(){
		return do_accumulate_(&DB_ADAPTER::sum);
	}

	auto do_min(){
		return do_accumulate_(&DB_ADAPTER::min);
	}

	auto do_max(){
		return do_accumulate_(&DB_ADAPTER::max);
	}

private:
	WorkerStatus do_set(){
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

	WorkerStatus do_setex(){
		const auto &p = protocol_.getParams();

		if (p.size() != 4)
			return err_BadRequest_();

		const auto &key = p[1];
		const auto &val = p[3];

		uint32_t exp = from_string<uint32_t>(p[2]);

		if (key.empty())
			return err_BadRequest_();

		db_.set(key, val, exp);

		protocol_.response_ok(buffer_);

		return WorkerStatus::WRITE;
	}

	WorkerStatus do_del(){
		const auto &p = protocol_.getParams();

		if (p.size() != 2)
			return err_BadRequest_();

		const auto &key = p[1];

		if (key.empty())
			return err_BadRequest_();

		protocol_.response_bool(buffer_, db_.del(key));

		return WorkerStatus::WRITE;
	}

	template<int Sign>
	WorkerStatus do_incrdecr(){
		const auto &p = protocol_.getParams();

		if (p.size() != 2 && p.size() != 3)
			return err_BadRequest_();

		const auto &key = p[1];

		if (key.empty())
			return err_BadRequest_();

		int64_t const val = p.size() == 3 ? Sign * from_string<int64_t>(p[2]) : Sign;

		if (val == 0)
			return err_BadRequest_();

		protocol_.response_string(buffer_, db_.incr(key, val));

		return WorkerStatus::WRITE;
	}

	auto do_incr(){
		return do_incrdecr<+1>();
	}

	auto do_decr(){
		return do_incrdecr<-1>();
	}

	auto do_getset(){
		const auto &p = protocol_.getParams();

		if (p.size() != 3)
			return err_BadRequest_();

		// GET

		const auto &key = p[1];

		if (key.empty())
			return err_BadRequest_();

		protocol_.response_string(buffer_, db_.get(key));
		// now old value is inserted in the buffer and
		// we do not care if pair is overwritten

		// SET

		const auto &val = p[2];

		db_.set(key, val);

		// return

		return WorkerStatus::WRITE;
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


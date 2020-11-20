namespace net{
namespace worker{



template<class Protocol, class DBAdapter, bool Mutable>
struct KeyValueWorkerExecCommand;



template<class Protocol, class DBAdapter>
auto KeyValueWorkerExecCommand<Protocol, DBAdapter, false>::operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer, const Command cmd) -> WorkerStatus {
	switch(cmd){

	case Command::EXIT	: return exit		(protocol, db, buffer);
	case Command::SHUTDOWN	: return shutdown	(protocol, db, buffer);

	case Command::INFO	: return info		(protocol, db, buffer);
	case Command::SAVE	: return save		(protocol, db, buffer);
	case Command::RELOAD	: return reload		(protocol, db, buffer);

	case Command::GET	: return get		(protocol, db, buffer);
	case Command::GETX	: return getx		(protocol, db, buffer);

	case Command::COUNT	: return count		(protocol, db, buffer);
	case Command::SUM	: return sum		(protocol, db, buffer);
	case Command::MIN	: return min		(protocol, db, buffer);
	case Command::MAX	: return max		(protocol, db, buffer);

	default			: return error::NotImplemented(protocol, buffer);
	}
}



template<class Protocol, class DBAdapter>
auto KeyValueWorkerExecCommand<Protocol, DBAdapter, true>::operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer, const Command cmd) -> WorkerStatus {
	switch(cmd){

	case Command::EXIT	: return exit		(protocol, db, buffer);
	case Command::SHUTDOWN	: return shutdown	(protocol, db, buffer);

	case Command::INFO	: return info		(protocol, db, buffer);
	case Command::SAVE	: return save		(protocol, db, buffer);
	case Command::RELOAD	: return reload		(protocol, db, buffer);

	case Command::GET	: return get		(protocol, db, buffer);
	case Command::GETX	: return getx		(protocol, db, buffer);

	case Command::COUNT	: return count		(protocol, db, buffer);
	case Command::SUM	: return sum		(protocol, db, buffer);
	case Command::MIN	: return min		(protocol, db, buffer);
	case Command::MAX	: return max		(protocol, db, buffer);

	case Command::SET	: return set		(protocol, db, buffer);
	case Command::SETEX	: return setex		(protocol, db, buffer);
	case Command::DEL	: return del		(protocol, db, buffer);
	case Command::GETSET	: return getset		(protocol, db, buffer);

	case Command::INCR	: return incr		(protocol, db, buffer);
	case Command::DECR	: return decr		(protocol, db, buffer);

	default			: return error::NotImplemented(protocol, buffer);
	}
}



// ====================================



template<class Protocol, class DBAdapter>
WorkerStatus KeyValueWorker<Protocol, DBAdapter>::operator()(IOBuffer &buffer){
	using Status  = protocol::ProtocolStatus;

	// PASS

	if (buffer.size() == 0)
		return WorkerStatus::PASS;

	const Status status = protocol_( std::string_view{ buffer } );

	if (status == Status::BUFFER_NOT_READ)
		return WorkerStatus::PASS;

	// ERROR

	buffer.clear();

	if (status == Status::ERROR)
		return error::InternalError(protocol_, buffer);

	// fetch command

	const auto &str = protocol_.getParams().front();

	const auto &cmd = RedisCommands::get(str);

	// EXEC

	using MyKeyValueWorkerExecCommand = KeyValueWorkerExecCommand<Protocol, DBAdapter, DBAdapter::MUTABLE>;

	MyKeyValueWorkerExecCommand exec;

	return exec(protocol_, db_, buffer, cmd);
}


} // namespace worker
} // namespace



namespace net{
namespace worker{


template<class CONNECTION>
WorkerStatus EchoWorker::operator()(CONNECTION &buffer){
	if (buffer.size() == 0)
		return WorkerStatus::PASS;

	// buffer.print();

	if (cmp_(buffer, cmd_shutdown	)){
		return WorkerStatus::SHUTDOWN;
	}

	if (cmp_(buffer, cmd_exit	)){
		return WorkerStatus::DISCONNECT;
	}


	if (cmp_(buffer, cmd_hello	)){
		buffer.clear();
		buffer.push("Hello, how are you?\r\n");
		return WorkerStatus::WRITE;
	}

	if (cmp_(buffer, cmd_help	)){
		buffer.clear();
		buffer.push(msg_help);
		return WorkerStatus::WRITE;
	}

	return WorkerStatus::WRITE;
}


} // namespace worker
} // namespace


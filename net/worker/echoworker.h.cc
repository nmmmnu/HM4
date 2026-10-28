
namespace net{
namespace worker{


WorkerStatus EchoWorker::operator()(IOBuffer<MemoryManager> &buffer){
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
		[[maybe_unused]]
		bool b = buffer.push_("Hello, how are you?\r\n");
		return WorkerStatus::WRITE;
	}

	if (cmp_(buffer, cmd_help	)){
		buffer.clear();
		[[maybe_unused]]
		bool b = buffer.push_(msg_help);
		return WorkerStatus::WRITE;
	}

	return WorkerStatus::WRITE;
}


} // namespace worker
} // namespace


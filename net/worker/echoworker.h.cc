
namespace net{
namespace worker{


WorkerStatus EchoWorker::operator()(IOBuffer<MemoryManager> &buffer, MemoryManager &mm){
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
		buffer.clear(mm);

		[[maybe_unused]]
		bool b = buffer.push(mm, "Hello, how are you?\r\n");

		return WorkerStatus::WRITE;
	}

	if (cmp_(buffer, cmd_help	)){
		buffer.clear(mm);

		[[maybe_unused]]
		bool b = buffer.push(mm, msg_help);

		return WorkerStatus::WRITE;
	}

	return WorkerStatus::WRITE;
}


} // namespace worker
} // namespace


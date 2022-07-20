#include <unistd.h>

int getProcessID(){
	return getpid();
}

#if 0
const std::string &getHostname(std::string &buffer){
	constexpr size_t size = 256;

	buffer.clear();
	buffer.resize(size);

	int result = sethostname(buffer.data(), buffer.size());

	if (result != 0)
		buffer.clear();



	return buffer;
}
#endif


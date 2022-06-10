#include "idgeneratordate.h"

#include "mytime.h"

#include <iomanip>
#include <sstream>

namespace hm4{
namespace idgenerator{


std::string IDGeneratorDate::operator()() const{
	std::ostringstream buff;

	constexpr const char *FORMAT = MyTime::TIME_FORMAT_NUMBER;

	auto const now = MyTime::now();

	buff	<< MyTime::toString(now, FORMAT)
		<< "."
		<< std::setfill('0') << std::setw(10)
		<< MyTime::toUsec(now);


	return buff.str();
}


} // namespace idgenerator
} // namespace

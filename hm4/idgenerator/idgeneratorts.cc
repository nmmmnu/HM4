#include "idgeneratorts.h"

#include "mytime.h"

#include <iomanip>
#include <sstream>

namespace hm4{
namespace idgenerator{


void IDGeneratorTS::format_(std::ostream &buff, uint32_t const value) const{
	if (hex_){
		buff	<< std::setfill('0') << std::setw(8) << std::hex;
	}else{
		buff	<< std::setfill('0') << std::setw(10);
	}

	buff << value;
}

std::string IDGeneratorTS::operator()() const{
	std::ostringstream buff;

	auto now = MyTime::now();

	format_(buff, MyTime::uncombine(now));

	buff << '.';

	format_(buff, MyTime::uncombine2(now));

	return buff.str();
}


} // namespace idgenerator
} // namespace

#include "idgeneratorts.h"

#include "mytime.h"

#include <iomanip>
#include <sstream>

namespace hm4{
namespace idgenerator{


void IDGeneratorTS::_format(std::ostream &buff, uint32_t value) const{
	if (_hex){
		buff	<< std::setfill('0') << std::setw(8) << std::hex;
	}else{
		buff	<< std::setfill('0') << std::setw(10);
	}

	buff << value;
}

std::string IDGeneratorTS::operator()() const{
	std::ostringstream buff;

	auto now = MyTime::now();

	_format(buff, MyTime::uncombine(now));

	buff << ".";

	_format(buff, MyTime::uncombine2(now));

	return buff.str();
}


} // namespace idgenerator
} // namespace

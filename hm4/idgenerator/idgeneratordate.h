#ifndef _ID_GENERATOR_DATE_H
#define _ID_GENERATOR_DATE_H

#include <string>
#include <ostream>
#include <cstdint>

namespace hm4{
namespace idgenerator{


class IDGeneratorDate{
public:
	std::string operator()() const;
};


} // namespace idgenerator
} // namespace

#endif


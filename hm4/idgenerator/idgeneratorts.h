#ifndef _ID_GENERATOR_TS_H
#define _ID_GENERATOR_TS_H

#include <string>
#include <ostream>
#include <cstdint>

namespace hm4{
namespace idgenerator{


class IDGeneratorTS{
public:
	IDGeneratorTS(bool const hex = true) : _hex(hex){};

	std::string operator()() const;

private:
	void _format(std::ostream &buff, uint32_t value) const;

private:
	bool _hex;
};


} // namespace idgenerator
} // namespace

#endif


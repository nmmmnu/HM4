#ifndef _ID_GENERATOR_TS_H
#define _ID_GENERATOR_TS_H

#include <string>
#include <ostream>
#include <cstdint>

namespace hm4{
namespace idgenerator{


class IDGeneratorTS{
public:
	IDGeneratorTS(bool const hex = true) : hex_(hex){};

	std::string operator()() const;

private:
	void format_(std::ostream &buff, uint32_t value) const;

private:
	bool hex_;
};


} // namespace idgenerator
} // namespace

#endif


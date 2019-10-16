#include "myglob.h"

#include <cassert>

bool MyGlob::open(std::string_view const path) noexcept{
	assert(isOpen_ == false);

	int const result = glob(path.data(), 0, nullptr, & globresults_);

	if (result != 0)
		return false;

	if (result == GLOB_NOMATCH)
		return false;

	isOpen_ = true;

	return true;
}

void MyGlob::close() noexcept{
	if (isOpen_ == false)
		return;

	globfree(& globresults_);

	isOpen_ = false;
}


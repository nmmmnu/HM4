#include "myglob.h"

#include <sys/stat.h>	// stat

MyGlob::MyGlob(MyGlob &&other) :
		globresults_	(std::move(other.globresults_	)),
		isOpen_		(std::move(other.isOpen_	)),
		data_		(std::move(other.data_		)){
	other.isOpen_ = false;
}

MyGlob& MyGlob::operator=(MyGlob &&other){
	if (isOpen_ || other.isOpen_)
		swap(other);

	return *this;
}

void MyGlob::swap(MyGlob &other){
	using std::swap;

	swap(globresults_,	other.globresults_	);
	swap(isOpen_,		other.isOpen_		);
	swap(data_,		other.data_		);
}

// ==============================

bool MyGlob::open(const StringRef &path) noexcept{
	if (isOpen_)
		close();

	if (open_(path.data(), globresults_) == false)
		return false;

	isOpen_ = true;


	size_t i;
	for(i = 0; i < globresults_.gl_pathc; ++i){
		const char *filename = globresults_.gl_pathv[i];

		if (checkFile_( filename ))
			data_.push_back(filename);
	}

	return i > 0;
}

bool MyGlob::open_(const char *path, glob_t &globresults) noexcept{
	int const result = glob(path, 0, nullptr, & globresults);

	if (result != 0)
		return false;

	if (result == GLOB_NOMATCH)
		return false;

	return true;
}

void MyGlob::close() noexcept{
	if (! isOpen_)
		return;

	data_.clear();

	globfree(& globresults_);

	isOpen_ = false;
}

bool MyGlob::checkFile_(const char *filename) noexcept{
	struct stat s;
	stat(filename, & s);

	if (s.st_mode & S_IFREG)	// file
		return true;

	if (s.st_mode & S_IFLNK)	// symlink
		return true;

	return false;
}


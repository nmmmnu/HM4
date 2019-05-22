#ifndef _MYGLOB_H
#define _MYGLOB_H

#include "stringref.h"

#include <glob.h>	// glob

#include <vector>

class MyGlob final{
public:
	using container_type = std::vector<StringRef>;

public:
	MyGlob() = default;

	MyGlob(const MyGlob &other) = delete;
	MyGlob& operator=(MyGlob other) = delete;

	MyGlob(MyGlob &&other) = default;
	MyGlob& operator=(MyGlob &&other) = default;

	~MyGlob() noexcept{
		if (isOpen_)
			close();
	}

public:
	bool open(const StringRef &path) noexcept;
	void close() noexcept;

	static bool isFile(const char *filename) noexcept;
public:
	size_t size() const noexcept{
		return globresults_.gl_pathc;
	}

	auto begin() const noexcept{
		return globresults_.gl_pathv;
	}

	auto end() const noexcept{
		return globresults_.gl_pathv + globresults_.gl_pathc;
	}

private:
	bool		isOpen_		= false;
	glob_t		globresults_;
	container_type	data_;
};

#endif


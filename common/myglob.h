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

	MyGlob(MyGlob &&other);
	MyGlob& operator=(MyGlob &&other);

	void swap(MyGlob &other);

	~MyGlob() noexcept{
		close();
	}

public:
	bool open(const StringRef &path) noexcept;
	void close() noexcept;

public:
	const StringRef &operator[](size_t const index) const{
		return data_[index];
	}

	container_type::size_type size() const{
		return data_.size();
	}

	container_type::const_iterator begin() const{
		return data_.begin();
	}

	container_type::const_iterator end() const{
		return data_.end();
	}

	container_type::const_reverse_iterator rbegin() const{
		return data_.rbegin();
	}

	container_type::const_reverse_iterator rend() const{
		return data_.rend();
	}

private:
	static bool open_(const char *path, glob_t &globresults) noexcept;
	static bool checkFile_(const char *filename) noexcept;

private:
	glob_t		globresults_;
	bool		isOpen_		= false;
	container_type	data_;
};

#endif


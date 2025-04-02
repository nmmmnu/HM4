#include "mmapfile.h"

int main(){
	MMAPFile mmap;

	mmap.create("test.bin", MMAPFile::Advice::NORMAL, 5ull * 1024 * 1024 * 1024);
}


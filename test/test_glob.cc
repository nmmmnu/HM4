#include "myglob.h"

#include <iostream>
#include <cstdio>

static int print_usage(const char *name){
	printf("Usage:\n");
	printf("\t%s [directory] - list directory using glob()\n", name);
	printf("\t\tDo not forget about quotes around the wildcards such '*'\n");

	return 1;
}

MyGlob globCreate(const char *path){
	MyGlob gl;
	gl.open(path);
	return gl;
}

int main(int argc, char **argv){
	if (argc < 2)
		return print_usage(argv[0]);

	const char *path = argv[1];

	const MyGlob gl = globCreate(path);

	for(const StringRef &s : gl)
		std::cout << s << '\n';
}


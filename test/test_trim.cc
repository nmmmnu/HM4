#include "trim.h"

#include <cstring>

#include "mytest.h"

MyTest mytest;

// ==================================

static void fn_trim();

// ==================================

int main(){
	fn_trim();

	return mytest.end();
}

// ==================================

constexpr size_t BUFFER_SIZE = 128;
char buffer[BUFFER_SIZE + 1];

static void fn_trim(const char *input, const char *result){
	size_t const input_size  = strlen(input);
	size_t const result_size = strlen(result);


	if (input_size > BUFFER_SIZE){
		printf("Input is over %zu characters\n", BUFFER_SIZE);
		exit(1);
	}

	memcpy(buffer, input, input_size);
	buffer[input_size] = '\0';

	size_t size2 = trim_size(buffer, input_size);

	size_t const size1 = strlen(buffer);

//	printf("%zu %zu %s %zu %s\n", size1, size2, buffer, result_size, result);

	mytest("trim", size1 == result_size && size2 == result_size && strcmp(buffer, result) == 0);
}

static void fn_trim(){
	mytest.begin("Standard");

	fn_trim("hello",	"hello"	);
	fn_trim("hello ",	"hello"	);
	fn_trim("hello\t",	"hello"	);
	fn_trim("hello\r",	"hello"	);
	fn_trim("hello\n",	"hello"	);
	fn_trim("hello \t\r\n",	"hello"	);

	mytest.begin("Empty");

	fn_trim("",		""	);
	fn_trim(" ",		""	);
	fn_trim("\t",		""	);
	fn_trim("\r",		""	);
	fn_trim("\n",		""	);
	fn_trim(" \t\r\n",	""	);
}


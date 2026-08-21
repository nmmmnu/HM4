#include "mybufferview.h"

#include <cstddef>

template <typename T>
struct CB{
	constexpr static T *data(){
		return nullptr;
	}

	constexpr static size_t size(){
		return 0;
	}
};


int main(){
	using namespace MyBuffer;

	{
		[[maybe_unused]]
		BufferView<int> a;
		[[maybe_unused]]
		BufferView<int> b = a;
	}
	{
		[[maybe_unused]]
		BufferView<int> a;
		[[maybe_unused]]
		BufferView<const int> c = a;
	}
	{
		[[maybe_unused]]
		CB<char> bin;
		[[maybe_unused]]
		BufferView<const int>	a{ bin.data(), bin.size(), from_bytes{} };
	}

	{
		[[maybe_unused]]
		CB<char const> bin;
		[[maybe_unused]]
		BufferView<const int>	a{ bin.data(), bin.size(), from_bytes{} };
	}

	{
		[[maybe_unused]]
		CB<char> bin;
		[[maybe_unused]]
		BufferView<int> 	a{ bin.data(), bin.size(), from_bytes{} };
	}

	// --------------

	{
		[[maybe_unused]]
		CB<void> bin;
		[[maybe_unused]]
		BufferView<const int>	a{ bin.data(), bin.size(), from_bytes{} };
	}

	{
		[[maybe_unused]]
		CB<void const> bin;
		[[maybe_unused]]
		BufferView<const int>	a{ bin.data(), bin.size(), from_bytes{} };
	}

	{
		[[maybe_unused]]
		CB<void> bin;
		[[maybe_unused]]
		BufferView<int>		a{ bin.data(), bin.size(), from_bytes{} };
	}

	// --------------

	{
		[[maybe_unused]]
		CB<int> bin;
		[[maybe_unused]]
		BufferView<int>		a{ bin };
	}

	{
		[[maybe_unused]]
		CB<const char> bin;
	//	BufferView<int>		a{ bin, from_bytes{} };
	}

	{
		[[maybe_unused]]
		CB<char> bin;
		[[maybe_unused]]
		BufferView<int>		a{ bin, from_bytes{} };
	}

	{
		[[maybe_unused]]
		CB<const char> bin;
		[[maybe_unused]]
		BufferView<const int>	a{ bin, from_bytes{} };
	}

	{
		[[maybe_unused]]
		CB<const char> bin;
	//	BufferView<int>	a{ bin, from_bytes{} };
	}
}


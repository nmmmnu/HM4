#include <iostream>

#include "stringref.h"
#include "binarysearch.h"

template <typename T>
struct MyCmdPair{
	StringRef	cmd;
	T		code;

	bool operator > (const MyCmdPair &other) const{
		return cmd.compare(other.cmd) > 0;
	}

	bool operator < (const MyCmdPair &other) const{
		return cmd.compare(other.cmd) < 0;
	}
};

struct BinarySearchCompCmdPair{
	template <class ARRAY, class SIZE, class KEY>
	int operator()(const ARRAY &list, SIZE const index, const KEY &key) const{
		return list[index].cmd.compare(key);
	}
};

template <class CONTAINER>
void find(const CONTAINER &commands, const StringRef &cmd){
	using size_type = typename CONTAINER::size_type;

	const auto &result = binarySearch(commands, size_type(0), commands.size(), cmd, BinarySearchCompCmdPair{});

	if (result.first){
		std::cout << result.second << '\n';
		std::cout << commands[result.second].cmd << '\n';
	}
}

#include <array>
#include <algorithm>

enum class Command{
	INFO,
	GET,
	SET
};

using CommandPair = MyCmdPair<Command>;

auto commandFactory(){
	std::array<CommandPair, 6> v{
		CommandPair{ "info",	Command::INFO	},
		CommandPair{ "INFO",	Command::INFO	},
		CommandPair{ "get",	Command::GET	},
		CommandPair{ "GET",	Command::GET	},
		CommandPair{ "set",	Command::SET	},
		CommandPair{ "SET",	Command::SET	}
	};

	std::sort(v.begin(), v.end());

	return v;
}

int main(){
	const auto commands = commandFactory();

	find(commands, "get");
}


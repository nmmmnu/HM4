#include "protocol/redisprotocol.h"

#include "mytest.h"

MyTest mytest;

using RedisProtocol = net::protocol::RedisProtocol;
using Status = RedisProtocol::Status;

static void test(RedisProtocol &p, const char *data, const std::initializer_list<StringRef> &v = {}){
	const Status status = p(data);

	bool const ok = v.size();

	if (status == Status::OK && ok){
		const auto &q = p.getParams();

		bool const result = std::equal(q.begin(), q.end(), v.begin(), v.end() );

		return mytest("", result);
	}

	if (status != Status::OK && ! ok)
		return mytest("", true);

	return mytest("", false);
}

int main(){
	RedisProtocol p;

	mytest.begin("Redis Protocol");

	test(p, "*3\r\n$3\r\nSET\r\n$4\r\ncity\r\n$5\r\nSofia\r\n"	, { "SET", "city", "Sofia"	}	);

	test(p, "*2\r\n$3\r\nGET\r\n$4\r\ncity\r\n"			, { "GET", "city"		}	);

	test(p, "*92\r\n$3\r\nGET\r\n$4\r\ncity\r\n"	);
	test(p, "$2\r\n$3\r\nGET\r\n$4\r\ncity\r\n"	);
	test(p, "*2\r\n*3\r\nGET\r\n$4\r\ncity\r\n"	);

	return mytest.end();
}

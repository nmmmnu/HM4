<?php

function cmd_HASH($redis){
	rawCommand($redis, "hdellall", "a");

	$redis->hmset("a", [
		"name"	=> "asus"	,
		"cpu"	=> "i7"		,
		"color"	=> "#FF0000"
	]);

	$redis->hset("a", "name", "dell");

	expect("HSET / HMSET",	true										);

	expect("HLEN",		$redis->hlen("a") == 3								);
	expect("HGET",		$redis->hget("a", "name") == "dell"						);
	expect("HEXISTS",	$redis->hexists("a", "name")							);

	$x = $redis->hmget("a", [ "name", "cpu", "color", "some" ]);
	expect("HMGET",		count($x) == 4 && $x["name"] == "dell" && $x["cpu"] == "i7" && $x["color"] == "#FF0000" && $x["some"] == ""	);

	$x = $redis->hgetall("a");
	expect("HGETALL",	count($x) == 3 && $x["name"] == "dell" && $x["cpu"] == "i7" && $x["color"] == "#FF0000"				);

	// strange no support in PHPRedis
	$x = rawCommand($redis, "hgetkeys", "a");
	expect("HGETKEYS",	count($x) == 3 && $x[0] == "color" && $x[1] == "cpu" && $x[2] == "name"		);

	// strange no support in PHPRedis
	$x = rawCommand($redis, "hgetvals", "a");
	expect("HGETVALS",	count($x) == 3 && $x[0] == "#FF0000" && $x[1] == "i7" && $x[2] == "dell"	);

	$redis->hdel("a", "name");
	expect("HDEL",		$redis->hlen("a") == 2 && !$redis->hexists("a", "name")				);

	rawCommand($redis, "hdelall", "a");

	expect("HDELALL",	$redis->hgetall("a") == []							);

	$redis->hmset("a", [
		"name"	=> "dell"	,
		"cpu"	=> "i7"		,
		"color"	=> "#FF0000"
	]);

	rawCommand($redis, "hexpireall", "a", 1);

	$x = $redis->hgetall("a");
	expect("HGETALL",	count($x) == 3 && $x["name"] == "dell" && $x["cpu"] == "i7" && $x["color"] == "#FF0000"				);

	pause(2);

	expect("HDELALL",	$redis->hgetall("a") == []							);

	rawCommand($redis, "hexpireall", "a", 1);
	rawCommand($redis, "hpersistall", "a");

	expect("HPERSISTALL",	$redis->ttl("a~name"	) == 0 &&
				$redis->ttl("a~cpu"	) == 0 &&
				$redis->ttl("a~color"	) == 0 &&
				true
	);
}


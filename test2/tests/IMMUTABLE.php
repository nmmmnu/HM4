<?php

function cmd_IMMUTABLE($redis){
	$redis->set("a", 1);
	$redis->set("b", 2);
	$redis->del("c");

	// ------------------

	expect("GET",		$redis->get("a") == 1			);
	expect("GET",		$redis->get("b") == 2			);
	expect("GET",		$redis->get("c") == ""			);

	// ------------------

	$m = $redis->mget(["a", "b", "c"]);

	expect("MGET",		$m[0] == 1				);
	expect("MGET",		$m[1] == 2				);
	expect("MGET",		$m[2] == ""				);

	// ------------------

	expect("EXISTS",	$redis->exists("a")			);
	expect("EXISTS",	$redis->exists("b")			);
	expect("EXISTS",	$redis->exists("c") == false		);

	// ------------------

	expect("EXPIRETIME",	true					);

	$redis->expire("a", 100);

	expect("TTL",		$redis->ttl("a") > 98	);
	expect("TTL",		$redis->ttl("b") == 0	);
	expect("TTL",		$redis->ttl("c") == 0	);

	// ------------------

	$s = "helloNEWworld";

	$redis->set("a", $s);

	expect("GETRANGE",	$redis->getrange("a", 5, 7) == "NEW"	);

	// ------------------

	expect("STRLEN",	$redis->strlen("a") == strlen($s)	);

	// ------------------

	$redis->del(["a", "b"]);
}


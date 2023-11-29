<?php

function cmd_MUTABLE_GET($redis){

	$redis->del("a");

	expect("GETSET",	$redis->getset("a", 2) == ""			);
	expect("GETSET",	$redis->get("a") == 2				);

	$redis->set("a", 1);

	expect("GETSET",	$redis->getset("a", 2) == 1			);
	expect("GETSET",	$redis->get("a") == 2				);

	// ------------------

	$redis->del("a");

	expect("GETDEL",	$redis->getdel("a") == ""			);
	expect("GETDEL",	$redis->exists("a") == false			);

	$redis->set("a", 1);

	expect("GETDEL",	$redis->getdel("a") == 1			);
	expect("GETDEL",	$redis->exists("a") == false			);

	// ------------------

	$redis->del("a");

	expect("GETEX",		rawCommand($redis, "getex", "a", 1) == ""	);
	expect("GETEX",		$redis->exists("a") == false			);

	expect("GETPERSIST",	rawCommand($redis, "getpersist", "a") == ""	);
	expect("GETEX",		$redis->exists("a") == false			);

	$redis->set("a", 1);
	$redis->setex("b", 1, 2);

	expect("GETEX",		rawCommand($redis, "getex", "a", 1) == 1	);
	expect("GETPERSIST",	rawCommand($redis, "getpersist", "b") == 2	);
	pause(2);
	expect("GETEX",		$redis->exists("a") == false			);
	expect("GETPERSIST",	$redis->get("b") == 2				);

	// ------------------

	$redis->del(["a", "b"]);
}


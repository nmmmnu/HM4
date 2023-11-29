<?php

function cmd_MUTABLE($redis){
	expect("SET",		true		);

	// ------------------

	$redis->mset([
		"a"	=> 1,
		"b"	=> 2,
	]);

	expect("MSET",		$redis->get("a") == 1			);
	expect("MSET",		$redis->get("b") == 2			);

	// ------------------

	$redis->del("c");
	$redis->del("d");
	$redis->del("e");

	$redis->msetnx([
		"c"	=> 1,
		"d"	=> 2,
	]);

	expect("MSETNX",	$redis->get("c") == 1			);
	expect("MSETNX",	$redis->get("d") == 2			);

	$redis->msetnx([
		"a"	=> 100,
		"e"	=> 2,
	]);

	expect("MSETNX",	$redis->get("a") == 1			);
	expect("MSETNX",	$redis->get("e") == ""			);

	// ------------------

	rawCommand($redis, "msetxx",
		"a", 10,
		"b", 20
	);

	expect("MSETXX",	$redis->get("a") == 10			);
	expect("MSETXX",	$redis->get("b") == 20			);

	// ------------------

	$redis->setex("a", 1, 5);
	$redis->expire("b", 1);
	$redis->expire("c", 1);
	$redis->persist("c");

	pause(2);

	expect("SETEX",		$redis->exists("a") == false		);
	expect("EXPIRE",	$redis->exists("b") == false		);
	expect("PERSIST",	$redis->exists("c")			);

	// ------------------

	$redis->del("a");
	$redis->set("b", 2);
	$redis->setnx("a", 1);
	$redis->setnx("b", 20);
	expect("SETNX",		$redis->get("a") == 1			);
	expect("SETNX",		$redis->get("b") == 2			);

	// ------------------

	$redis->del("a");
	$redis->set("b", 2);
	rawCommand($redis, "setxx", "a", 10);
	rawCommand($redis, "setxx", "b", 20);

	expect("SETXX",		$redis->exists("a") == false		);
	expect("SETXX",		$redis->get("b") == 20			);

	// ------------------

	$redis->set("a", "hello");
	$redis->append("a", "world");

	expect("APPEND",	$redis->get("a") == "helloworld"	);

	// ------------------

	expect("DEL",		true		);

	// ------------------

	$redis->del(["a", "b", "c", "d", "e"]);
}


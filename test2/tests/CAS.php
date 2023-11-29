<?php

function cmd_CAS($redis){
	$redis->del("a");

	$val = rawCommand($redis, "cas", "a", 1, 2);

	expect("CAS",		!$val					);

	$redis->set("a", 1);

	$val = rawCommand($redis, "cas", "a", 1, 2);

	expect("CAS",		$val && $redis->get("a") == 2		);

	$redis->set("a", 3);

	$val = rawCommand($redis, "cas", "a", 1, 2);

	expect("CAS",		$val == false && $redis->get("a") == 3	);

	// ------------------

	$redis->del("a");

	$val = rawCommand($redis, "cad", "a", 1);

	expect("CAD",		!$val					);

	$redis->set("a", 1);

	$val = rawCommand($redis, "cad", "a", 1);

	expect("CAD",		$val && $redis->exists("a") == false	);

	$redis->set("a", 2);

	$val = rawCommand($redis, "cad", "a", 1);

	expect("CAD",		$val == false && $redis->get("a") == 2	);

	// ------------------

	$redis->del(["a"]);
}


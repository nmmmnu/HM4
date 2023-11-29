<?php

function cmd_COPY($redis){
	// COPY 404

	$redis->del("a");
	$redis->del("b");

	$val = rawCommand($redis, "copy", "a", "b");

	expect("COPY",		!$val					);

	// ------------------

	// COPY OK

	$redis->set("a", 1);
	$redis->del("b");

	$val = rawCommand($redis, "copy", "a", "b");

	expect("COPY",		$val					);
	expect("COPY",		$redis->get("a") == 1			);
	expect("COPY",		$redis->get("b") == 1			);

	// ------------------



	// COPY NX 404


	$redis->del("a");
	$redis->del("b");

	$val = rawCommand($redis, "copynx", "a", "b");

	expect("COPYNX",	!$val					);

	// ------------------

	// COPY NX OK

	$redis->set("a", 1);
	$redis->del("b");

	$val = rawCommand($redis, "copynx", "a", "b");

	expect("COPYNX",	$val					);
	expect("COPYNX",	$redis->get("a") == 1			);
	expect("COPYNX",	$redis->get("b") == 1			);

	// ------------------

	// COPY NX OVERWRITE

	$redis->set("a", 1);
	$redis->set("b", 2);

	$val = rawCommand($redis, "copynx", "a", "b");

	expect("COPYNX",	!$val					);
	expect("COPYNX",	$redis->get("a") == 1			);
	expect("COPYNX",	$redis->get("b") == 2			);

	// ------------------



	// MOVE 404

	$redis->del("a");
	$redis->del("b");

	$val = rawCommand($redis, "move", "a", "b");

	expect("MOVE",		!$val					);

	// ------------------

	// MOVE

	$redis->set("a", 1);
	$redis->del("b");

	$val = rawCommand($redis, "move", "a", "b");

	expect("MOVE",		$val					);
	expect("MOVE",		$redis->exists("a") == false		);
	expect("MOVE",		$redis->get("b") == 1			);

	// ------------------



	// MOVE NX 404

	$redis->del("a");
	$redis->del("b");

	$val = rawCommand($redis, "movenx", "a", "b");

	expect("MOVENX",	!$val					);

	// ------------------

	// MOVE NX OK

	$redis->set("a", 1);
	$redis->del("b");

	$val = rawCommand($redis, "movenx", "a", "b");

	expect("MOVENX",	$val					);
	expect("MOVENX",	$redis->exists("a") == false		);
	expect("MOVENX",	$redis->get("b") == 1			);

	// ------------------

	// MOVE NX OVERWRITE

	$redis->set("a", 1);
	$redis->set("b", 2);

	$val = rawCommand($redis, "movenx", "a", "b");

	expect("MOVENX",	!$val					);
	expect("MOVENX",	$redis->exists("a")			);
	expect("MOVENX",	$redis->get("b") == 2			);

	// ------------------



	$redis->del(["a", "b"]);
}


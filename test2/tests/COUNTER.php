<?php

function cmd_COUNTER($redis){
	$redis->del("a");

	expect("INCR",		$redis->incr("a")	== 1	);
	expect("INCR",		$redis->incr("a")	== 2	);
	expect("INCR",		$redis->incr("a")	== 3	);
	expect("INCR",		$redis->incr("a")	== 4	);

	// ------------------

	expect("INCRBY",	$redis->incrby("a", 2)	== 6	);
	expect("INCRBY",	$redis->incrby("a", 2)	== 8	);
	expect("INCRBY",	$redis->incrby("a", 2)	== 10	);

	// ------------------

	expect("INCRBY",	$redis->incrby("a", -2)	== 8	);
	expect("INCRBY",	$redis->incrby("a", -2)	== 6	);
	expect("INCRBY",	$redis->incrby("a", -2)	== 4	);

	// ------------------

	expect("INCRBY",	$redis->incr("a", -1)	== 3	);
	expect("INCRBY",	$redis->incr("a", -1)	== 2	);
	expect("INCRBY",	$redis->incr("a", -1)	== 1	);
	expect("INCRBY",	$redis->incr("a", -1)	== 0	);

	// ------------------

	$redis->del("a");

	expect("DECR",		$redis->decr("a")	== -1	);
	expect("DECR",		$redis->decr("a")	== -2	);
	expect("DECR",		$redis->decr("a")	== -3	);
	expect("DECR",		$redis->decr("a")	== -4	);

	// ------------------

	expect("DECRBY",	$redis->decrby("a", 2)	== -6	);
	expect("DECRBY",	$redis->decrby("a", 2)	== -8	);
	expect("DECRBY",	$redis->decrby("a", 2)	== -10	);

	// ------------------

	expect("DECRBY",	$redis->decrby("a", -2)	== -8	);
	expect("DECRBY",	$redis->decrby("a", -2)	== -6	);
	expect("DECRBY",	$redis->decrby("a", -2)	== -4	);

	// ------------------

	expect("DECRBY",	$redis->decrby("a", -1)	== -3	);
	expect("DECRBY",	$redis->decrby("a", -1)	== -2	);
	expect("DECRBY",	$redis->decrby("a", -1)	== -1	);
	expect("DECRBY",	$redis->decrby("a", -1)	== 0	);

	// ------------------

	$redis->del(["a"]);
}


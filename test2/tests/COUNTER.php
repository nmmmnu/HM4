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

	// ------------------

	expect("INCRTO",	rawCommand($redis, "incrto", "a", 100	) == 100	);
	expect("INCRTO",	rawCommand($redis, "incrto", "a", 105	) == 105	);
	expect("INCRTO",	rawCommand($redis, "incrto", "a", 115	) == 115	);
	expect("INCRTO",	rawCommand($redis, "incrto", "a", 110	) == 115	);
	expect("INCRTO",	rawCommand($redis, "incrto", "a", 100	) == 115	);
	expect("INCRTO",	rawCommand($redis, "incrto", "a", 200	) == 200	);

	$redis->del(["a"]);

	expect("DECRTO",	rawCommand($redis, "decrto", "a", 900	) == 900	);
	expect("DECRTO",	rawCommand($redis, "decrto", "a", 150	) == 150	);
	expect("DECRTO",	rawCommand($redis, "decrto", "a", 130	) == 130	);
	expect("DECRTO",	rawCommand($redis, "decrto", "a", 100	) == 100	);
	expect("DECRTO",	rawCommand($redis, "decrto", "a", 150	) == 100	);
	expect("DECRTO",	rawCommand($redis, "decrto", "a", 130	) == 100	);


	// ------------------

	$redis->del(["a"]);
}


<?php

function cmd_BITSET($redis){
	$redis->del("a");
	$redis->del("b");

	expect("BITGET",	$redis->getbit("a", 5		) == 0		);

	$redis->setbit("a", 5, 1);

	expect("BITGET",	$redis->getbit("a", 5		) == 1		);

	$redis->setbit("a", 5, 0);

	expect("BITGET",	$redis->getbit("a", 5		) == 0		);

	expect("BITGET",	$redis->getbit("a", 5000	) == 0		);

	// Not supported by PHPRedis
	rawCommand($redis, "bitset", "a", 5, 1, 6, 1);

	expect("BITGET",	$redis->getbit("a", 5		) == 1		);
	expect("BITGET",	$redis->getbit("a", 6		) == 1		);

	// Not supported by PHPRedis
	$x = rawCommand($redis, "bitmget", "a", 1, 5, 6);

	expect("BITMGET",	$x[0] == 0 && $x[1] == 1 && $x[2] == 1		);

	$x = rawCommand($redis, "bitmget", "b", 1, 5, 6);

	expect("BITMGET",	$x[0] == 0 && $x[1] == 0 && $x[2] == 0		);

	expect("BITSET",	true	);
	expect("BITMSET",	true	);

	// Not supported by PHPRedis. It fails even on normal Redis
	expect("BITCOUNT",	rawCommand($redis, "bitcount", "a") == 2	);

	$redis->del("a");
}


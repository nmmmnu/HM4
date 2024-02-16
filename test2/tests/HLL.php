<?php

function cmd_HLL($redis){
	$redis->del("a");
	$redis->del("b");
	$redis->del("c");
	$redis->del("d");

	expect("PFCOUNT",	$redis->pfcount("a") == 0		);

	rawCommand($redis, "PFRESERVE", "a");

	expect("PFRESERVE",	true);
	expect("PFCOUNT",	$redis->pfcount("a") == 0		);

	$redis->pfadd("a", ["Sofia"] );
	expect("PFCOUNT",	$redis->pfcount("a") == 1		);

	rawCommand($redis, "PFRESERVE", "a");

	$redis->pfadd("a", ["Varna"] );
	expect("PFCOUNT",	$redis->pfcount("a") == 2		);

	$redis->pfadd("a", ["Bourgas"] );
	expect("PFCOUNT",	$redis->pfcount("a") == 3		);

	$redis->pfadd("a", ["New York", "London"] );
	expect("PFCOUNT",	$redis->pfcount("a") == 5		);

	$redis->pfadd("b", ["New York", "London", "Kiev", "Los Angeles"] );

	expect("PFCOUNT",	$redis->pfcount(["a", "b"]) == 7	);

	$redis->pfmerge("c", ["a", "b"]);
	expect("PFMERGE",	true					);

	expect("PFCOUNT",	$redis->pfcount("c") == 7		);

	expect("PFADD",		true					);

	$count = rawCommand($redis, "pfintersect", "a", "b");
	expect("PFINTERSECT",	$count == 2				);

	$count = rawCommand($redis, "pfintersect", "a", "b", "c");
	expect("PFINTERSECT",	$count == 2				);

	$redis->pfadd("d", ["New York"]					);

	$count = rawCommand($redis, "pfintersect", "a", "b", "c", "d");
	expect("PFINTERSECT",	$count == 1				);

	$redis->del("a");
	$redis->del("b");
	$redis->del("c");
	$redis->del("d");
}


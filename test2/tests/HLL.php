<?php

function cmd_HLL($redis){
	$redis->del("a");
	$redis->del("b");
	$redis->del("c");
	$redis->del("d");

	expect("PFCOUNT",	$redis->pfcount("a") == 0				);

	rawCommand($redis, "PFRESERVE", "a");

	expect("PFRESERVE",	true							);
	expect("PFCOUNT",	$redis->pfcount("a") == 0				);

	expect("PFADD",		$redis->pfadd("a", ["Sofia"] ) == 1			);
	expect("PFCOUNT",	$redis->pfcount("a") == 1				);

	expect("PFADD",		$redis->pfadd("a", ["Sofia"] ) == 0			);
	expect("PFCOUNT",	$redis->pfcount("a") == 1				);

	rawCommand($redis, "PFRESERVE", "a");

	expect("PFADD",		$redis->pfadd("a", ["Varna"] ) == 1			);
	expect("PFCOUNT",	$redis->pfcount("a") == 2				);

	expect("PFADD",		$redis->pfadd("a", ["Varna"] ) == 0			);
	expect("PFCOUNT",	$redis->pfcount("a") == 2				);

	expect("PFADD",		$redis->pfadd("a", ["Bourgas"] ) == 1			);
	expect("PFCOUNT",	$redis->pfcount("a") == 3				);

	expect("PFADD",		$redis->pfadd("a", ["Bourgas"] ) == 0			);
	expect("PFCOUNT",	$redis->pfcount("a") == 3				);

	expect("PFADD",		$redis->pfadd("a", ["New York", "London"] ) == 1	);
	expect("PFCOUNT",	$redis->pfcount("a") == 5				);

	expect("PFADD",		$redis->pfadd("a", ["New York", "London"] ) == 0	);
	expect("PFCOUNT",	$redis->pfcount("a") == 5				);



	$count = rawCommand($redis, "pfaddcount", "b", "New York", "London", "Kiev", "Los Angeles");

	expect("PFADDCOUNT",	$count == 4						);

	$count = rawCommand($redis, "pfaddcount", "b", "New York");

	expect("PFADDCOUNT",	$count == 4						);



	expect("PFCOUNT",	$redis->pfcount(["a", "b"]) == 7			);



	$redis->pfmerge("c", ["a", "b"]);
	expect("PFMERGE",	true							);

	expect("PFCOUNT",	$redis->pfcount("c") == 7				);

	expect("PFADD",		true							);

	$count = rawCommand($redis, "pfintersect", "a", "b");
	expect("PFINTERSECT",	$count == 2						);

	$count = rawCommand($redis, "pfintersect", "a", "b", "c");
	expect("PFINTERSECT",	$count == 2						);

	$redis->pfadd("d", ["New York"]							);

	$count = rawCommand($redis, "pfintersect", "a", "b", "c", "d");
	expect("PFINTERSECT",	$count == 1						);

	$redis->del("a");
	$redis->del("b");
	$redis->del("c");
	$redis->del("d");
}


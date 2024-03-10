<?php

function cmd_CBF($redis){
	$redis->del("a");
	$redis->del("b");

	$w	= 4096;
	$d	= 4;

	foreach([8, 16, 32, 64] as $is){
		rawCommand($redis, "CBFRESERVE $is", "a", $w, $d, $is);

		expect("CBFADDCOUNT $is", rawCommand($redis, "CBFADDCOUNT", "a", $w, $d, $is, "Sofia", 1) == 1 );
		expect("CBFADDCOUNT $is", rawCommand($redis, "CBFADDCOUNT", "a", $w, $d, $is, "Sofia", 4) == 5 );

		rawCommand($redis, "CBFREM", "a", $w, $d, $is, "Sofia", 1);

		expect("CBFREMCOUNT $is", rawCommand($redis, "CBFREMCOUNT", "a", $w, $d, $is, "Sofia", 1) == 3 );

		rawCommand($redis, "CBFADD", "a", $w, $d, $is, "New York", 4);
		rawCommand($redis, "CBFADD", "a", $w, $d, $is, "New York", 1, "London", 1, "Kiev", 1, "Los Angeles", 44, "Sofia", 7);

		expect("CBFADD $is",		true				);

		rawCommand($redis, "CBFRESERVE $is", "a", $w, $d, $is);

		expect("CBFCOUNT $is",	rawCommand($redis, "CBFCOUNT", "a", $w, $d, $is, "Sofia"	) == 10	);
		expect("CBFCOUNT $is",	rawCommand($redis, "CBFCOUNT", "a", $w, $d, $is, "New York"	) == 5	);
		expect("CBFCOUNT $is",	rawCommand($redis, "CBFCOUNT", "a", $w, $d, $is, "Rome"		) == 0	);
		expect("CBFCOUNT $is",	rawCommand($redis, "CBFCOUNT", "b", $w, $d, $is, "Rome"		) == 0	);

		$m  = rawCommand($redis, "CBFMCOUNT", "a", $w, $d, $is, "Sofia", "New York", "Rome");

		expect("CBFMCOUNT $is",	$m[0] == 10 && $m[1] == 5 && $m[2] == 0	);

		$m  = rawCommand($redis, "CBFMCOUNT", "b", $w, $d, $is, "New York", "London", "York");

		expect("CBFMCOUNT $is",	!$m[0] && !$m[1] && !$m[2]		);

		$redis->del("a");
	}
}


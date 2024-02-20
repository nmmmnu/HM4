<?php

function cmd_CMS($redis){
	$redis->del("a");
	$redis->del("b");

	$w	= 4096;
	$d	= 4;

	foreach([8, 16, 32, 64] as $is){
		rawCommand($redis, "CMSRESERVE $is", "a", $w, $d, $is);

		rawCommand($redis, "CMSADD", "a", $w, $d, $is, "Sofia", 1);
		rawCommand($redis, "CMSADD", "a", $w, $d, $is, "Sofia", 4);
		rawCommand($redis, "CMSADD", "a", $w, $d, $is, "New York", 4);
		rawCommand($redis, "CMSADD", "a", $w, $d, $is, "New York", 1, "London", 1, "Kiev", 1, "Los Angeles", 44, "Sofia", 5);

		expect("CMSADD $is",		true				);

		rawCommand($redis, "CMSRESERVE $is", "a", $w, $d, $is);

		expect("CMSCOUNT $is",	rawCommand($redis, "CMSCOUNT", "a", $w, $d, $is, "Sofia"	) == 10	);
		expect("CMSCOUNT $is",	rawCommand($redis, "CMSCOUNT", "a", $w, $d, $is, "New York"	) == 5	);
		expect("CMSCOUNT $is",	rawCommand($redis, "CMSCOUNT", "a", $w, $d, $is, "Rome"		) == 0	);
		expect("CMSCOUNT $is",	rawCommand($redis, "CMSCOUNT", "b", $w, $d, $is, "Rome"		) == 0	);

		$m  = rawCommand($redis, "CMSMCOUNT", "a", $w, $d, $is, "Sofia", "New York", "Rome");

		expect("CMSMCOUNT $is",	$m[0] == 10 && $m[1] == 5 && $m[2] == 0	);

		$m  = rawCommand($redis, "CMSMCOUNT", "b", $w, $d, $is, "New York", "London", "York");

		expect("CMSMCOUNT $is",	!$m[0] && !$m[1] && !$m[2]		);

		$redis->del("a");
	}
}


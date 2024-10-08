<?php

function cmd_TD($redis){
	$redis->del("a");
	$redis->del("b");
	$redis->del("c");

	$size  = 100;
	$delta = 0.05;

	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile", "a", $size, 0.50) == 0	);
	expect("TDSIZE",	rawCommand($redis, "tdpercentile", "a", $size) == 0		);

	rawCommand($redis, "TDRESERVE", "a", $size);

	expect("PFRESERVE",	true								);
	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile", "a", $size, 0.50) == 0	);
	expect("TDSIZE",	rawCommand($redis, "tdsize",       "a", $size) == 0		);

	rawCommand($redis, "tdadd", "a", $size, $delta, 100);

	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile", "a", $size, 0.50) == 100	);
	expect("TDSIZE",	rawCommand($redis, "tdsize",       "a", $size) == 1		);

	rawCommand($redis, "TDRESERVE", "a", $size);

	rawCommand($redis, "tdaddweight", "a", $size, $delta ,
							 80, 1000,
							110, 1000
	);

	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile", "a", $size, 0.50) == 100	);
	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile", "a", $size, 0.90) == 110	);
	expect("TDSIZE",	rawCommand($redis, "tdsize",       "a", $size) == 2001		);

	expect("TDMIN",		rawCommand($redis, "tdmin",        "a", $size) == 80		);
	expect("TDMIN",		rawCommand($redis, "tdmax",        "a", $size) == 110		);

	rawCommand($redis, "tdmerge", "b", $size, $delta, "a");

	expect("TDMERGE",	true								);

	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile", "b", $size, 0.50) == 100	);
	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile", "b", $size, 0.90) == 110	);
	expect("TDSIZE",	rawCommand($redis, "tdsize",       "b", $size) == 2001		);

	rawCommand($redis, "tdmerge", "b", $size, $delta, "a", "a");

	expect("TDMERGE",	true								);

	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile", "b", $size, 0.50) == 100	);
	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile", "b", $size, 0.90) == 110	);
	expect("TDSIZE",	rawCommand($redis, "tdsize",       "b", $size) == 6003		);

	$csize = 10;

	rawCommand($redis, "tdmergecapacity", "c", $csize, $delta, "b", $size, "a", $size);

	expect("TDMERGECAPACITY",	true							);

	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile", "c", $csize, 0.50) == 100	);
	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile", "c", $csize, 0.90) == 110	);
	expect("TDSIZE",	rawCommand($redis, "tdsize",       "c", $csize) == 8004		);

	$redis->del("a");
	$redis->del("b");
	$redis->del("c");
}


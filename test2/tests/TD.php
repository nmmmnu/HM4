<?php

function cmd_TD($redis){
	$redis->del("a");
	$redis->del("b");
	$redis->del("c");

	$size  = 100;
	$delta = 0.05;

	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile",  "a", $size, 0.50) == 0	);
	expect("TDSIZE",	rawCommand($redis, "tdpercentile",  "a", $size) == 0		);

	rawCommand($redis, "TDRESERVE", "a", $size);

	expect("PFRESERVE",	true				  				);
	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile",  "a", $size, 0.50) == 0	);
	expect("TDSIZE",	rawCommand($redis, "tdsize",        "a", $size) == 0		);

	rawCommand($redis, "tdadd", "a", $size, $delta, 100);

	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile",  "a", $size, 0.50) == 100	);
	expect("TDSIZE",	rawCommand($redis, "tdsize",        "a", $size) == 1		);

	rawCommand($redis, "TDRESERVE", "a", $size);

	rawCommand($redis, "tdaddweight", "a", $size, $delta ,
							 80, 1000,
							110, 1000
	);

	expect("TDMEDIAN",	rawCommand($redis, "tdmedian",      "a", $size      ) == 100	);
	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile",  "a", $size, 0.50) == 100	);
	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile",  "a", $size, 0.90) == 110	);
	expect("TDMPERCENTILE", rawCommand($redis, "tdmpercentile", "a", $size, 0.50, 0.90) == [ 100, 110 ]	);
	expect("TDSIZE",	rawCommand($redis, "tdsize",        "a", $size) == 2001		);

	expect("TDMIN",		rawCommand($redis, "tdmin",         "a", $size) == 80		);
	expect("TDMIN",		rawCommand($redis, "tdmax",         "a", $size) == 110		);

	rawCommand($redis, "tdmerge", "b", $size, $delta, "a");

	expect("TDMERGE",	true								);

	expect("TDMEDIAN",	rawCommand($redis, "tdmedian",      "b", $size      ) == 100	);
	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile",  "b", $size, 0.50) == 100	);
	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile",  "b", $size, 0.90) == 110	);
	expect("TDMPERCENTILE", rawCommand($redis, "tdmpercentile", "b", $size, 0.50, 0.90) == [ 100, 110 ]	);
	expect("TDSIZE",	rawCommand($redis, "tdsize",        "b", $size) == 2001		);

	rawCommand($redis, "tdmerge", "b", $size, $delta, "a", "a");

	expect("TDMERGE",	true								);

	expect("TDMEDIAN",	rawCommand($redis, "tdmedian",      "b", $size      ) == 100	);
	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile",  "b", $size, 0.50) == 100	);
	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile",  "b", $size, 0.90) == 110	);
	expect("TDMPERCENTILE", rawCommand($redis, "tdmpercentile", "b", $size, 0.50, 0.90) == [ 100, 110 ]	);
	expect("TDSIZE",	rawCommand($redis, "tdsize",        "b", $size) == 6003		);

	$csize = 10;

	rawCommand($redis, "tdmergecapacity", "c", $csize, $delta,  "b", $size, "a", $size);

	expect("TDMERGECAPACITY",	true							);

	expect("TDMEDIAN",	rawCommand($redis, "tdmedian",      "c", $csize      ) == 100	);
	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile",  "c", $csize, 0.50) == 100	);
	expect("TDPERCENTILE",	rawCommand($redis, "tdpercentile",  "c", $csize, 0.90) == 110	);
	expect("TDMPERCENTILE", rawCommand($redis, "tdmpercentile", "c", $csize, 0.50, 0.90) == [ 100, 110 ]	);
	expect("TDSIZE",	rawCommand($redis, "tdsize",        "c", $csize) == 8004	);

	$redis->del("a");

	rawCommand($redis, "tdaddweight", "a", $size, $delta ,
							1, 100,
							2, 100,
							3, 100,
							4, 100,
							5, 100
	);

	expect("TDMEAN",	rawCommand($redis, "tdmean",        "a", $size			) == 3			);

	expect("TDTRIMMEDMEAN",	rawCommand($redis, "tdtrimmedmean", "a", $size, 0.00, 1.00	) == 3			);
	expect("TDTRIMMEDMEAN",	rawCommand($redis, "tdtrimmedmean", "a", $size, 0.49, 0.51	) == 3			);
	expect("TDTRIMMEDMEAN",	rawCommand($redis, "tdtrimmedmean", "a", $size, 0.20, 1.00	) == (2+3+4+5) / 4	);
	expect("TDTRIMMEDMEAN",	rawCommand($redis, "tdtrimmedmean", "a", $size, 0.20, 0.80	) == (2+3+4) / 3	);
	expect("TDTRIMMEDMEAN",	rawCommand($redis, "tdtrimmedmean", "a", $size, 0.40, 0.80	) == (3+4) / 2		);
	expect("TDTRIMMEDMEAN",	rawCommand($redis, "tdtrimmedmean", "a", $size, 0.60, 0.80	) == (4) / 1		);

	$redis->del("a");
	$redis->del("b");
	$redis->del("c");
}


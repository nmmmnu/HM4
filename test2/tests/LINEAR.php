<?php

function cmd_LINEAR($redis){
	// clean up.
	rawCommand($redis, "xndel", "linear", "linear");



	$max_key = 4;
	$max = 50;

	// build matrix

	for($key = 0; $key < $max_key; ++$key){
		for($x = 0; $x < $max; ++$x){
			$ukey = "m:$key:$x";
			$val  = $x;
			rawCommand($redis, "mc1add", "linear", $ukey, $x, $val);
		}
	}

	// update some
	rawCommand($redis, "mc1add", "linear",
					"m:3:4", 6, 6,
					"m:3:5", 6, 6
	);

	// delete some
	rawCommand($redis, "mc1rem", "linear", "m:3:6");

	// get
	expect("MC1REM",	rawCommand($redis, "mc1get",		"linear", "m:3:4"				) == "6"		);
	expect("MC1GET",	rawCommand($redis, "mc1get",		"linear", "m:0:1"				) == "1"		);

	expect("MC1MGET",	rawCommand($redis, "mc1mget",		"linear", "m:0:2", "m:1:2", "nonexistent"	) == [ "2", "2", "" ]	);

	expect("MC1EXISTS",	rawCommand($redis, "mc1exists",		"linear", "m:3:4"				)			);
	expect("MC1EXISTS",	rawCommand($redis, "mc1exists",		"linear", "m:0:2"				)			);
	expect("MC1EXISTS",	rawCommand($redis, "mc1exists",		"linear", "nonexistent"				) == false		);

	expect("MC1SCORE",	rawCommand($redis, "mc1score",		"linear", "m:3:5"				) == [ 6 ]		);
	expect("MC1SCORE",	rawCommand($redis, "mc1score",		"linear", "m:0:2"				) == [ 2 ]		);

	$result = [
		"00000000000000000006,00000000", 6,
		"00000000000000000006,00000001", 6,
		"00000000000000000006,00000002", 6,
		"00000000000000000006,00000003", 6,
		"00000000000000000006,00000004", 6,
		""
	];

	expect("MC1POINT",	rawCommand($redis, "mc1point",		"linear", 6, 1000				) == $result		);

	$result = [
		"00000000000000000006,00000000", 6,
		"00000000000000000006,00000001", 6,
		"00000000000000000006,00000002", 6,
		"00000000000000000006,00000003", 6,
		"00000000000000000006,00000004", 6,
		"00000000000000000007,00000005", 7,
		"00000000000000000007,00000006", 7,
		"00000000000000000007,00000007", 7,
		"00000000000000000007,00000008", 7,
		"00000000000000000008,00000009", 8,
		"00000000000000000008,0000000a", 8,
		"00000000000000000008,0000000b", 8,
		"00000000000000000008,0000000c", 8,
		""
	];

	expect("MC1RANGE",	rawCommand($redis, "mc1range",		"linear", 6, 8, 1000			) == $result				);

	// unspecified, crash test
	rawCommand($redis, "mc1decode", "FFjunk");
	rawCommand($redis, "mc1decode", "junkFF");



	rawCommand($redis, "xndel", "linear", "linear");
}


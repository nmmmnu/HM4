<?php

function cmd_SL($redis) {
	$redis->del("a");

	expect("SLGETALL",	rawCommand($redis, "slgetall", "a") == []	);
	expect("SLCOUNT",	rawCommand($redis, "slcount", "a") == 0	);

	rawCommand($redis, "sladd", "a", "Germany", "Poland");

	expect("SLCOUNT",	rawCommand($redis, "slcount", "a") == 2		);

	rawCommand($redis, "sladd", "a", "USA", "UK", "Bulgaria");

	expect("SLCOUNT",	rawCommand($redis, "slcount", "a") == 5		);

	expect("SLGETALL",	rawCommand($redis, "slgetall", "a") == ["Germany", "Poland", "USA", "UK", "Bulgaria"]	);

	expect("SLGET",		rawCommand($redis, "slget", "a", 1) == "Poland"	);
	expect("SLMGET",	rawCommand($redis, "slmget", "a", 1, 3, 33 ) == ["Poland", "UK", ""]	);

	$redis->del("a");



	for($i = 0; $i < 250; ++$i){
		rawCommand($redis, "sladd", "a", $i);
	}

	expect("SLMGET",	rawCommand($redis, "slmget", "a", 1    ) == [1]				);	// naive
	expect("SLMGET",	rawCommand($redis, "slmget", "a", 1, 2, 3, 4, 5 ) == [1, 2, 3, 4, 5]	);	// small

	for($i = 0; $i < 250; ++$i){
		rawCommand($redis, "sladd", "a", $i);
	}

	expect("SLMGET",	rawCommand($redis, "slmget", "a", 1, 2, 3, 4, 5 ) == [1, 2, 3, 4, 5]	);	// huge

	$redis->del("a");



	$redis->set("a", 1);	// invalid input

	expect("SLCOUNT",	rawCommand($redis, "slcount", "a") == 0		);

	expect("SLGETALL",	rawCommand($redis, "slgetall", "a") == []	);

	expect("SLGET",		rawCommand($redis, "slget", "a", 1) == ""	);
	expect("SLMGET",	rawCommand($redis, "slmget", "a", 1, 3, 33 ) == ["", "", ""]	);

	$redis->del("a");
}

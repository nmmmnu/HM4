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

	$redis->del("a");
}

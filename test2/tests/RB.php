<?php

function cmd_RB($redis){
	$redis->del("a");

	foreach([16, 32, 40, 64, 128, 256, 512, 1024] as $bytes){
		$size = 5;

		expect("RBGET", rawCommand($redis, "rbget", "a", $size, $bytes) == []);

		rawCommand($redis, "rbadd", "a", $size, $bytes, 1, 2, 3, 4);

		$res = rawCommand($redis, "rbget", "a", $size, $bytes);
		expect("RBGET_INIT_$bytes", $res == [1, 2, 3, 4]);

		rawCommand($redis, "rbadd", "a", $size, $bytes, 6, 7, 8, 9);

		$res = rawCommand($redis, "rbget", "a", $size, $bytes);
		expect("RBGET", $res == [4, 6, 7, 8, 9]);

		expect("RBPOP", rawCommand($redis, "rbpop", "a", $size, $bytes) == 4);
		expect("RBPOP", rawCommand($redis, "rbpop", "a", $size, $bytes) == 6);
		expect("RBPOP", rawCommand($redis, "rbpop", "a", $size, $bytes) == 7);
		expect("RBPOP", rawCommand($redis, "rbpop", "a", $size, $bytes) == 8);
		expect("RBPOP", rawCommand($redis, "rbpop", "a", $size, $bytes) == 9);

		expect("RBPOP", rawCommand($redis, "rbpop", "a", $size, $bytes) == "");
		expect("RBPOP", rawCommand($redis, "rbpop", "a", $size, $bytes) == "");


		$redis->del("a");

		rawCommand($redis, "rbadd", "a", $size, $bytes, 1, 2, 3, 4);
		expect("RBCOUNT", rawCommand($redis, "rbcount", "a", $size, $bytes) == 4);

		rawCommand($redis, "rbadd", "a", $size, $bytes, 1, 2, 3, 4);
		expect("RBCOUNT", rawCommand($redis, "rbcount", "a", $size, $bytes) == 5);

		expect("RBPOP", rawCommand($redis, "rbpop", "a", $size, $bytes) == 4);
		expect("RBPOP", rawCommand($redis, "rbpop", "a", $size, $bytes) == 1);

		expect("RBCOUNT_AFTER_POPS_$bytes", rawCommand($redis, "rbcount", "a", $size, $bytes) == 3);

		$redis->del("a");
	}

	$redis->del("a");
}



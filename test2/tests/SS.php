<?php

function cmd_SS($redis){
	$redis->del("a");
	$redis->del("b");
	$redis->del("c");
	$redis->del("mm");

	rawCommand($redis, "ssreserve", "mm");

	$allZeroes = [
		"count",	0,
		"open",		0,
		"close",	0,
		"first",	0,
		"last",		0,
		"min",		0,
		"max",		0,
		"sum",		0,
		"sumsq",	0,
		"sum2",		0,
		"range",	0,
		"change",	0,
		"avg",		0,
		"harm",		0,
		"geom",		0,
		"vari",		0,
		"sdev",		0,
		"rms",		0,
		"cvar",		0
	];

	expect("SSGETALL",	rawCommand($redis, "ssgetall",	"mm"			) == $allZeroes);
	expect("SSGET",		rawCommand($redis, "ssget",	"mm", "min"		) == 0);
	expect("SSMGET",	rawCommand($redis, "ssmget",	"mm", "min", "max"	) == [ 0, 0 ]);

	rawCommand($redis, "ssadd", "a", "1");
	rawCommand($redis, "ssadd", "a", "1");

	expect("SSGETALL", rawCommand($redis, "ssgetall", "a") == [
		"count",	2,
		"open",		1,
		"close",	1,
		"first",	1,
		"last",		1,
		"min",		1,
		"max",		1,
		"sum",		2,
		"sumsq",	2,
		"sum2", 	2,
		"range",	0,
		"change",	0,
		"avg",		1,
		"harm",		1,
		"geom",		1,
		"vari",		0,
		"sdev",		0,
		"rms",		1,
		"cvar",		0
	]);

	rawCommand($redis, "ssadd", "b", "2");
	rawCommand($redis, "ssadd", "b", "2");

	expect("SSGET",			rawCommand($redis, "ssget",  "b", "sum"		) == 4);
	expect("SSMGET",		rawCommand($redis, "ssmget", "b", "min", "max"	) == [2,2]);
	expect("SSGETALL_MERGE",	rawCommand($redis, "ssgetall", "a", "b"	) == [
		"count",	 4.0000000000,
		"open",   	 1.0000000000,
		"close",	 2.0000000000,
		"first",	 1.0000000000,
		"last",   	 2.0000000000,
		"min",   	 1.0000000000,
		"max",   	 2.0000000000,
		"sum",  	 6.0000000000,
		"sumsq",	10.0000000000,
		"sum2",		10.0000000000,
		"range",	 1.0000000000,
		"change", 	 1.0000000000,
		"avg",   	 1.5000000000,
		"harm",   	 1.3333333333,
		"geom",		 1.4142135624,
		"vari", 	 0.2500000000,
		"sdev",   	 0.5000000000,
		"rms",   	 1.5811388301,
		"cvar",   	 0.3333333333
	]);

	expect("SSGETALL_MERGE_BA", rawCommand($redis, "ssgetall", "b", "a") == rawCommand($redis, "ssgetall", "a", "b"));

	rawCommand($redis, "ssmerge", "c", "a", "b");

	expect("SSGETALL_C", rawCommand($redis, "ssgetall", "c") == rawCommand($redis, "ssgetall", "a", "b"));

	$redis->del("a");

	expect("SSADDGET",	rawCommand($redis, "ssaddget",		"a", "10", "sum"		) == "+10.0000000000");
	expect("SSADDMGET",	rawCommand($redis, "ssaddmget",		"a", "20", "count", "sum"	) == [ "+2.0000000000", "+30.0000000000" ]);
	expect("SSADDGETALL",	rawCommand($redis, "ssaddgetall",	"a", "30"			) == [
		"count",	   3.0000000000,
		"open",		  10.0000000000,
		"close",	  30.0000000000,
		"first",	  10.0000000000,
		"last",		  30.0000000000,
		"min",		  10.0000000000,
		"max",		  30.0000000000,
		"sum",		  60.0000000000,
		"sumsq",	1400.0000000000,
		"sum2",		1400.0000000000,
		"range",	  20.0000000000,
		"change",	  20.0000000000,
		"avg",		  20.0000000000,
		"harm",		  16.3636363636,
		"geom",		  18.1712059283,
		"vari",		  66.6666666667,
		"sdev",		   8.1649658093,
		"rms",		  21.6024689947,
		"cvar",		   0.4082482905
	]);

	$redis->del("a");
	$redis->del("b");
	$redis->del("c");
	$redis->del("mm");
}


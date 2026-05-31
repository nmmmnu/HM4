<?php

function cmd_ISAM($redis){
	$redis->del("a");
	$redis->del("b");

//	goto big;

	$mask = "3:id,10:name,2:country";

	expect("ISETALL",  rawCommand($redis, "isetall", $mask, "a", "1", "Niki", "BG") == 1);
	expect("GET", $redis->get("a") == "1  Niki      BG");

	expect("IGETALL",  rawCommand($redis, "igetall", $mask, "a") == [
		"id",		"1",
		"name",		"Niki",
		"country",	"BG"
	]);

	expect("IGETKEYS", rawCommand($redis, "igetkeys", $mask, "a") == ["id", "name", "country"]);
	expect("IGETKEYS", rawCommand($redis, "igetkeys", $mask, "b") == ["id", "name", "country"]);
	expect("IGETVALS", rawCommand($redis, "igetvals", $mask, "a") == ["1", "Niki", "BG"]);
	expect("IGETVALS", rawCommand($redis, "igetvals", $mask, "b") == []);

	expect("IMGET",    rawCommand($redis, "imget", $mask, "a", "name") == ["Niki"]);
	expect("IMGET",    rawCommand($redis, "imget", $mask, "a", "name", "country", "id") == ["Niki", "BG", "1"]);
	expect("IMGET",    rawCommand($redis, "imget", $mask, "a", "name", "country", "id", "id", "non_existent", "id") == [
		"Niki",
		"BG",
		"1",
		"1",
		"",
		"1"
	]);

	expect("ISET",     rawCommand($redis, "iset", $mask, "a", "name", "John") == 1);
	expect("GET", $redis->get("a") == "1  John      BG");

	expect("ISET",    rawCommand($redis, "iset", $mask, "a", "name", "Niki", "id", "5") == 1);
	expect("GET", $redis->get("a") == "5  Niki      BG");

	expect("IDEL",    rawCommand($redis, "idel", $mask, "a", "country", "id") == 1);
	expect("GET_RAW_AFTER_IDEL", $redis->get("a") == "   Niki        ");

	// =====================

big:

	$mask = "3:id,10:name,1:0,1:1,1:2,1:3,1:4,1:5,1:6,1:7,1:8,1:9";

	expect("ISETALL",	rawCommand($redis, "isetall", $mask, "b", 42, "Alex", 0,1,2,3,4,5,6,7,8,9) == 1);
	expect("IGETVALS",	rawCommand($redis, "igetvals", $mask, "b"			) == [42, "Alex", 0,1,2,3,4,5,6,7,8,9	]);

	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 0			) == [0				]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 0,1			) == [0, 1			]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 0,1,2			) == [0, 1,2			]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 0,1,2,3			) == [0, 1,2,3			]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 0,1,2,3,4		) == [0, 1,2,3,4		]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 0,1,2,3,4,5		) == [0, 1,2,3,4,5		]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 0,1,2,3,4,5,6		) == [0, 1,2,3,4,5,6		]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 0,1,2,3,4,5,6,7		) == [0, 1,2,3,4,5,6,7		]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 0,1,2,3,4,5,6,7,8	) == [0, 1,2,3,4,5,6,7,8	]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 0,1,2,3,4,5,6,7,8,9	) == [0, 1,2,3,4,5,6,7,8,9	]);

	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 9			) == [9				]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 9,8			) == [9,8			]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 9,8,7			) == [9,8,7			]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 9,8,7,6			) == [9,8,7,6			]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 9,8,7,6,5		) == [9,8,7,6,5			]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 9,8,7,6,5,4		) == [9,8,7,6,5,4		]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 9,8,7,6,5,4,3		) == [9,8,7,6,5,4,3		]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 9,8,7,6,5,4,3,2		) == [9,8,7,6,5,4,3,2		]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 9,8,7,6,5,4,3,2,1	) == [9,8,7,6,5,4,3,2,1		]);
	expect("IMGET",   	rawCommand($redis, "imget", $mask, "b", 9,8,7,6,5,4,3,2,1,0	) == [9,8,7,6,5,4,3,2,1,0	]);


	// =====================

	$redis->del("a");
	$redis->del("b");
}

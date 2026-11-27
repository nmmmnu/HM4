<?php

function cmd_PIECE($redis){
	$redis->del("a");

	expect("PIECESETALL",  rawCommand($redis, "piecesetall", "a", ",", "1", "Niki", "BG") == 1);
	expect("GET", $redis->get("a") == "1,Niki,BG");

	expect("PIECEGETALL",  rawCommand($redis, "piecegetall", "a", ",") == [
		1,
		"Niki",
		"BG"
	]);

	expect("PIECEGET",     rawCommand($redis, "pieceget", "a", ",", 1) == "Niki");
	expect("PIECEGET",     rawCommand($redis, "pieceget", "a", ",", 8) == "");

	expect("PIECEEXISTS",  rawCommand($redis, "pieceexists", "a", ",", 1) == 1);
	expect("PIECEEXISTS",  rawCommand($redis, "pieceexists", "a", ",", 8) == 0);

	expect("PIECEMGET",    rawCommand($redis, "piecemget", "a", ",", 1) == ["Niki"]);
	expect("PIECEMGET",    rawCommand($redis, "piecemget", "a", ",", 1, 2, 0, 8) == ["Niki", "BG", 1, ""]);

	expect("PIECEMEXISTS", rawCommand($redis, "piecemexists", "a", ",", 1) == [1]);
	expect("PIECEMEXISTS", rawCommand($redis, "piecemexists", "a", ",", 1, 2, 0, 8) == [1, 1, 1, 0]);


	expect("PIECESET",     rawCommand($redis, "pieceset", "a", ",", 1, "John") == 1);
	expect("GET", $redis->get("a") == "1,John,BG");

	expect("PIECESET",     rawCommand($redis, "pieceset", "a", ",", 1, "Edward") == 1);
	expect("GET", $redis->get("a") == "1,Edward,BG");

	expect("PIECEMSET",    rawCommand($redis, "pieceset", "a", ",", 1, "John") == 1);
	expect("GET", $redis->get("a") == "1,John,BG");

	expect("PIECEMSET",    rawCommand($redis, "pieceset", "a", ",", 1, "Lui", 2, "UK", 5, "XX") == 1);
	expect("GET", $redis->get("a") == "1,Lui,UK,,,XX");

	expect("PIECEDEL",     rawCommand($redis, "piecedel", "a", ",", 1) == 1);
	expect("GET", $redis->get("a") == "1,,UK,,,XX");

	expect("PIECEDEL",     rawCommand($redis, "piecedel", "a", ",", 2, 5) == 1);
	expect("GET", $redis->get("a") == "1");

	expect("PIECEDEL",     rawCommand($redis, "piecedel", "a", ",", 8) == 1);
	expect("GET", $redis->get("a") == "1");

	$redis->set("a", "a,b,"		);	rawCommand($redis, "pieceset",		"a", ",", 0, "xx"		);	expect("GET", $redis->get("a") == "xx,b"	);
	$redis->set("a", "a,b,,,"	);	rawCommand($redis, "piecedel",		"a", ",", 1			);	expect("GET", $redis->get("a") == "a"		);
	$redis->set("a", "a,b,,,"	);	rawCommand($redis, "pieceappend",	"a", ",", "c"			);	expect("GET", $redis->get("a") == "a,b,c"	);
	$redis->set("a", "a,b,c"	);	rawCommand($redis, "pieceset",		"a", ",", 1, "x", 1, "y"	);	expect("GET", $redis->get("a") == "a,y,c"	);
	$redis->set("a", "a,b,c"	);	rawCommand($redis, "pieceset",		"a", ",", 2, "", 0, "x"		);	expect("GET", $redis->get("a") == "x,b"		);
	$redis->set("a", "a,b,c"	);	rawCommand($redis, "piecedel",		"a", ",", 1, 1, 2		);	expect("GET", $redis->get("a") == "a"		);
//	$redis->set("a", "a,b"		);	rawCommand($redis, "pieceset",		"a", ",", 1000000, "x"		);	expect("GET", $redis->get("a") == "a,b"		);
	$redis->set("a", "a,b"		);	rawCommand($redis, "pieceset",		"a", ",", -5, "x"		);	expect("GET", $redis->get("a") == "x,b"		);

	$redis->del("a");
}

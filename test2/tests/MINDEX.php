<?php

function cmd_MINDEX($redis){
	rawCommand($redis, "xndel", "a", "a");

	expect("IXMADD",	rawCommand($redis, "ixmadd", "a", "01", "9999", " ", "space telescope discovers new earth sized planet"		) == 1);
	expect("IXMADD",	rawCommand($redis, "ixmadd", "a", "02", "9998", " ", "climate summit agrees on global clean energy transition"	) == 1);
	expect("IXMADD",	rawCommand($redis, "ixmadd", "a", "03", "9997", " ", "new battery technology doubles electric car range"	) == 1);
	expect("IXMADD",	rawCommand($redis, "ixmadd", "a", "04", "9996", " ", "autonomous telescope rover finds ice on mars planet"	) == 1);
	expect("IXMADD",	rawCommand($redis, "ixmadd", "a", "05", "9995", " ", "ai predicts extreme climate and clean water shortages"	) == 1);
	expect("IXMADD",	rawCommand($redis, "ixmadd", "a", "06", "9994", " ", "scientists test nuclear battery for deep space probe"	) == 1);

	$res = rawCommand($redis, "ixmsim1", "a",     "space", "", "");
	expect("IXMSIM1",	$res === ["06", "1", "01", "1", ""]);

	$res = rawCommand($redis, "ixmsim", "a", ",", "space", "", "");
	expect("IXMSIM",	$res === ["06", "1", "01", "1", ""]);

	$res = rawCommand($redis, "ixmsim", "a", ",", "space,planet", "", "");
	expect("IXMSIM",	$res === ["01", "1", ""]);

	$res = rawCommand($redis, "ixmsim", "a", ",", "climate,clean", "", "");
	expect("IXMSIM",	$res === ["05", "1", "02", "1", ""]);

	$res = rawCommand($redis, "ixmsim", "a", ",", "space,nuclear,battery", "", "");
	expect("IXMSIM",	$res === ["06", "1", ""]);

	$res = rawCommand($redis, "ixmsim", "a", ",", "space,ne", "", "");
	expect("IXMSIM",	$res === [""]);

	$res = rawCommand($redis, "ixmsim", "a", ",", "nonexistenttoken", "", "");
	expect("IXMSIM",	$res === [""]);

	expect("IXMREM",	rawCommand($redis, "ixmrem", "a", "01", "02", "03", "04", "05", "06") == 1);

	// Проверка след изтриване
	$res = rawCommand($redis, "ixmsim", "a", ",", "space", "", "");
	expect("IXMSIM",	$res === [""]);



	$tags  = "tag1,tag2,tag3,tag4,tag5,tag6";
	$tags1 = "tag1";

	for ($i = 1; $i <= 15; ++$i) {
		$id = sprintf("%02d", $i);
		$sortKey = sprintf("%04d", 9999 - $i);
		rawCommand($redis, "ixmadd", "a", $id, $sortKey, ",", $tags);
	}



	$p1 = rawCommand($redis, "ixmsim", "a", ",", $tags, 10, "");
	expect("IXMSIM",	count($p1) == (10 * 2 + 1));
	expect("IXMSIM",	$p1[0] == "15" && $p1[18] == "06");
	$cursor = end($p1);
	expect("IXMSIM",	$cursor == "9994~05");

	$p2 = rawCommand($redis, "ixmsim", "a", ",", $tags, 10, $cursor);
	expect("IXMSIM",	count($p2) == (5 * 2 + 1));
	expect("IXMSIM",	$p2[0] == "05" && $p2[8] == "01");
	$cursor = end($p2);
	expect("IXMSIM",	$cursor === "");



	$p1 = rawCommand($redis, "ixmsim1", "a", $tags1, 10, "");
	expect("IXMSIM1",	count($p1) == (10 * 2 + 1));
	expect("IXMSIM1",	$p1[0] == "15" && $p1[18] == "06");
	$cursor = end($p1);
	expect("IXMSIM1",	$cursor == "9994~05");

	$p2 = rawCommand($redis, "ixmsim1", "a", $tags1, 10, $cursor);
	expect("IXMSIM1",	count($p2) == (5 * 2 + 1));
	expect("IXMSIM1",	$p2[0] == "05" && $p2[8] == "01");
	$cursor = end($p2);
	expect("IXMSIM1",	$cursor === "");



	rawCommand($redis, "xndel", "a", "a");
}


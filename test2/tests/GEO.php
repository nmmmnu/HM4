<?php

function cmd_GEO($redis){
	// clean up.
	rawCommand($redis, "xndel", "places", "places");



	// add restaurants in Sofia
	$redis->geoadd(
		"places",
		42.69210669738513, 23.32489318092748, "Level Up"		,
		42.69238509863727, 23.32476304836964, "Luchiano"		,
		42.69175842097124, 23.32523636032996, "Bohemian Hall"		,
		42.69269382372572, 23.32491086796810, "Umamido"			,
		42.69199298885176, 23.32411779452691, "McDonald's"		,
		42.69141486008349, 23.32517469905046, "Ceiba"			,
		42.69210516803149, 23.32626916568055, "Happy"			,
		42.69151957227103, 23.32366568780231, "Ugo"			,
		42.69151957227103, 23.32654779876320, "Il Theatro"		,
		42.69205716858902, 23.32266435016691, "Nicolas"			,
		42.68970992959111, 23.32455331244497, "Manastirska Magernitsa"	,
		42.69325388808822, 23.32118939756011, "The Hadjidragana Tavern"	,
		42.69174997126510, 23.32100561258516, "Boho"			,
		42.69205885495273, 23.33781274367048, "Stastlivetza"		,
		42.68999839888917, 23.31970366848597, "Franco's Pizza"		,
		42.69074017093236, 23.31785351529929, "Villa Rosiche"		,
		42.68342508736217, 23.31633975360111, "Chevermeto"		,
		10.00000000000000, 10.00000000000000, "TO_BE_DELETED_0"		,
		10.00000000000000, 10.00000000000000, "TO_BE_DELETED_1"		,
	);



	// remove some restaurants
	rawCommand($redis, "georem", "places",
			"TO_BE_DELETED_0",
			"TO_BE_DELETED_1"
	);



	// get some place
	expect("GEOREM", rawCommand($redis, "geoget", "places", "TO_BE_DELETED_0"	) == ""							);
	expect("GEOGET", rawCommand($redis, "geoget", "places", "Boho"			) == "+42.6917499713,+23.3210056126,sx8dfevc6z40"	);

	$x = rawCommand($redis, "geomget", "places", "Boho", "Ugo", "MISSING");

	expect("GEOMGET",
				$x[0] == "+42.6917499713,+23.3210056126,sx8dfevc6z40"	&&
				$x[1] == "+42.6915195723,+23.3236656878,sx8dfezb0u19"	&&
				$x[2] == ""
	);



	// encode
	expect("GEOENCODE", rawCommand($redis, "geoencode", +42.6917499713, +23.3210056126)	== "+42.6917499713,+23.3210056126,sx8dfevc6z40"	);
	expect("GEOENCODE", rawCommand($redis, "geoencode", +42.6915195723, +23.3236656878)	== "+42.6915195723,+23.3236656878,sx8dfezb0u19"	);



	// decode, center of the box, so is bit different.
	expect("GEODECODE", rawCommand($redis, "geodecode", "sx8dfevc6z40")			== "+42.6917500142,+23.3210055716,sx8dfevc6z40"	);
	expect("GEODECODE", rawCommand($redis, "geodecode", "sx8dfezb0u19")			== "+42.6915195119,+23.3236656524,sx8dfezb0u19"	);



	// distance from Boho to Chevermeto
	// on Earth only :), in meters only

	$x = $redis->geodist("places", "Boho", "Chevermeto");

	// "real" distance on GoogleMaps - 1050 meters
	$xc = 1050;

	expect("GEODIST", $x > $xc - 50 && $x < $xc + 50 );



	// find all restaurants around some coordinates
	// on Earth only :), in meters only
	// not PHPRedis compatible

	$x = rawCommand($redis, "georadius", "places", 42.6921, 23.3248, 100);

	$e = [
		"McDonald's",		57,
		"Bohemian Hall",	52,
		"Level Up",		7,
		"Luchiano",		31,
		"Umamido",		66,
		"Ceiba",		82,
	];

	expect("GEORADIUS", $x == $e );



	rawCommand($redis, "xndel", "places", "places");
}


<?php

function hh_sort($data){
	$result = array();

	for ($i = 0; $i < count($data); ++$i)
		$result[$data[$i]] = $data[++$i];

	unset($data);

	asort($result);

	return $result;
}

function cmd_HH($redis){
	$redis->del("a");

	rawCommand($redis, "hhincr", "a", 4,
				"London"	,	20,
				"Sofia"		,	15,
				"Varna"		,	10,
				"NY"		,	 2,
				"LA"		,	90,
				"Boston"	,	-5
	);

	rawCommand($redis, "hhreserve", "a", 4);

	rawCommand($redis, "hhincr", "a", 4, "Sofia",  5);
	rawCommand($redis, "hhincr", "a", 4, "Sofia",  5);
	rawCommand($redis, "hhincr", "a", 4, "Sofia", 25);
	rawCommand($redis, "hhincr", "a", 4, "Sofia", 25);
	rawCommand($redis, "hhincr", "a", 4, "Sofia", 38);

	rawCommand($redis, "hhincr", "a", 4, "London", 25);

	rawCommand($redis, "hhreserve", "a", 4);

	$result = [
	    "Varna"		=> 10,
	    "London"		=> 25,
	    "Sofia"		=> 38,
	    "LA"		=> 90
	];

	$_ = hh_sort(rawCommand($redis, "hhget", "a", 4));

//	print_r($_);

	expect("HHINCR",	$_ == $result		);



	$redis->del("a");

	rawCommand($redis, "hhdecr", "a", 4,
				"London"	,	20,
				"Sofia"		,	15,
				"Varna"		,	10,
				"NY"		,	 2,
				"LA"		,	90,
				"Boston"	,	-5
	);

	rawCommand($redis, "hhreserve", "a", 4);

	rawCommand($redis, "hhdecr", "a", 4, "Sofia", 10);
	rawCommand($redis, "hhdecr", "a", 4, "Sofia", 10);
	rawCommand($redis, "hhdecr", "a", 4, "Sofia",  8);
	rawCommand($redis, "hhdecr", "a", 4, "Sofia",  3);
	rawCommand($redis, "hhdecr", "a", 4, "Sofia",  0);

	rawCommand($redis, "hhdecr", "a", 4, "London", 12);

	rawCommand($redis, "hhreserve", "a", 4);

	$result = [
		"Boston"	=> -5,
		"Sofia"		=>  0,
		"NY"		=>  2,
		"Varna"		=> 10,
	];

	$_ = hh_sort(rawCommand($redis, "hhget", "a", 4));

//	print_r($_);

	expect("HHDECR",	$_ == $result		);



	$redis->del("a");
}


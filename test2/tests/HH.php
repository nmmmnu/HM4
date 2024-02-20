<?php

function hh_sort($data){
	if (!is_array($data))
		return ["error, not an array"];

	$result = [];

	for ($i = 0; $i < count($data); ++$i)
		$result[$data[$i]] = $data[++$i];

	unset($data);

	asort($result);

	return $result;
}

function cmd_HH($redis){
	$redis->del("a");

	foreach([32, 40, 64, 128, 256] as $bytes){
		rawCommand($redis, "hhincr", "a", 4, $bytes,
					"London"	,	20,
					"Sofia"		,	15,
					"Varna"		,	10,
					"NY"		,	 2,
					"LA"		,	90,
					"Boston"	,	-5
		);

		rawCommand($redis, "hhreserve", "a", 4, $bytes);

		rawCommand($redis, "hhincr", "a", 4, $bytes, "Sofia",  5);
		rawCommand($redis, "hhincr", "a", 4, $bytes, "Sofia",  5);
		rawCommand($redis, "hhincr", "a", 4, $bytes, "Sofia", 25);
		rawCommand($redis, "hhincr", "a", 4, $bytes, "Sofia", 25);
		rawCommand($redis, "hhincr", "a", 4, $bytes, "Sofia", 38);

		rawCommand($redis, "hhincr", "a", 4, $bytes, "London", 25);

		rawCommand($redis, "hhreserve", "a", 4, $bytes);

		$result = [
		    "Varna"		=> 10,
		    "London"		=> 25,
		    "Sofia"		=> 38,
		    "LA"		=> 90
		];

		$_ = hh_sort(rawCommand($redis, "hhget", "a", 4, $bytes));

	//	print_r($_);

		expect("HHINCR$bytes",	$_ == $result		);



		$redis->del("a");

		rawCommand($redis, "hhdecr", "a", 4, $bytes,
					"London"	,	20,
					"Sofia"		,	15,
					"Varna"		,	10,
					"NY"		,	 2,
					"LA"		,	90,
					"Boston"	,	-5
		);

		rawCommand($redis, "hhreserve", "a", 4, $bytes);

		rawCommand($redis, "hhdecr", "a", 4, $bytes, "Sofia", 10);
		rawCommand($redis, "hhdecr", "a", 4, $bytes, "Sofia", 10);
		rawCommand($redis, "hhdecr", "a", 4, $bytes, "Sofia",  8);
		rawCommand($redis, "hhdecr", "a", 4, $bytes, "Sofia",  3);
		rawCommand($redis, "hhdecr", "a", 4, $bytes, "Sofia",  0);

		rawCommand($redis, "hhdecr", "a", 4, $bytes, "London", 12);

		rawCommand($redis, "hhreserve", "a", 4, $bytes);

		$result = [
			"Boston"	=> -5,
			"Sofia"		=>  0,
			"NY"		=>  2,
			"Varna"		=> 10,
		];

		$_ = hh_sort(rawCommand($redis, "hhget", "a", 4, $bytes));

	//	print_r($_);

		expect("HHDECR$bytes",	$_ == $result		);
	}


	$redis->del("a");
}


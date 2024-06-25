<?php
function rs_fill($redis, $count, $bytes, $distinct_items){
	for($i = 0; $i < $count * 10; ++$i){
		$v = $i % $distinct_items;
		rawCommand($redis, "rsadd", "a", $count, $bytes,
				$v, $v, $v, $v, $v,
				$v, $v, $v, $v, $v
		);
	}
}

function rs_makeIdealArray($count, $distinct_items){
	$x = [];

	for($i = 0; $i < $distinct_items; ++$i)
		$x[] = $i;

	return $x;
}

function cmd_RS($redis){
	$redis->del("a");

	$count		= 100;
	$max_tries	= 20;
	$distinct_items	= 25;

	$idealArray	= rs_makeIdealArray($count, $distinct_items);

	foreach([16, 32, 40, 64, 128, 256] as $bytes){
		$tries = 0;

	// label for goto
	again:
		$ok = true;

		rawCommand($redis, "rsreserve", "a", $count, $bytes);
		rs_fill($redis, $count, $bytes, $distinct_items);
		rawCommand($redis, "rsreserve", "a", $count, $bytes);

		$x = rawCommand($redis, "rsget", "a", $count, $bytes);
		$x = array_unique($x);
		sort($x);

	//	print_r($x);

		if ($x != $idealArray)
			if (++$tries < $max_tries){
				printf("RS %3d distribution not ideal, will try again...\n", $bytes);
				goto again;
			}else{
				expect("RS$bytes", false);
			}

		expect("RS$bytes", true);
	}

	$redis->del("a");
}


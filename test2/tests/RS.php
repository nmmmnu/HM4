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

function rs_makeModelArray($count, $distinct_items){
	$x = [];

	for($i = 0; $i < $distinct_items; ++$i)
		$x[] = $i;

	return $x;
}

function cmd_RS($redis){
	$redis->del("a");

	$count		= 100;
	$max_tries	= 10;
	$distinct_items	= 25;

	$modelArray	= rs_makeModelArray($count, $distinct_items);

	foreach([16, 32, 40, 64, 128, 256] as $bytes){
		rawCommand($redis, "rsreserve", "a", $count, $bytes);

		rs_fill($redis, $count, $bytes, $distinct_items);

		rawCommand($redis, "rsreserve", "a", $count, $bytes);

		// var $ok
		$ok = true;

		for($tries = 0; $tries < $max_tries; ++$tries){
			$ok = true;

			$x = rawCommand($redis, "rsget", "a", $count, $bytes);
			$x = array_unique($x);
			$x = sort($x);

			if ($x == $modelArray){
				expect("RS$bytes", true);
				break;
			}else{
				printf("RS %3d %5d distribution strange, will try again...\n", $bytes, $i);
				$ok = false;
			}
		}

		if (!$ok)
			expect("RS$bytes", false);
	}

	$redis->del("a");
}


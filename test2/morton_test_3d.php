<?php

function rawCommand(){
	$args = func_get_args();

	$redis = $args[0];

	array_shift($args);

	// PHPRedis only
	return call_user_func_array( [ $redis, "rawCommand" ], $args );
}



$redis = new Redis();
$redis->connect("127.0.0.1");



$results = 64000;

$cmd = ! true ? "MC3RANGE" : "MC3RANGENAIVE";

$next = "";

$id = 0;

do{
	$x = rawCommand($redis, $cmd, "morton3d",
					10, 14, // x
					 4,  8, // y
					26, 28, // z
					$results, $next
	);

	for($i = 0; $i < count($x) - 1; $i+=2){
		$k = $x[$i + 0];
		$v = $x[$i + 1];

		printf("| %10d | %-20s | %-20s |\n", $id, $k, $v);

		++$id;
	}

	$next = end($x);
}while($next);


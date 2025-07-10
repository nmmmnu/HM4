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



$max = 100;

for($z = 0; $z < $max; ++$z){
	for($y = 0; $y < $max; ++$y){
		for($x = 0; $x < $max; ++$x){
			$sub = "aaa.$x.$y.$z";
			rawCommand($redis, "MC3ADD", "morton3d", $sub, $x, $y, $z, $sub);

			$sub = "bbb.$x.$y.$z";
			rawCommand($redis, "MC3ADD", "morton3d", $sub, $x, $y, $z, $sub);
		}
	}

	printf("%5d of %5d\r", $z, $max);
}

printf("done      \n");


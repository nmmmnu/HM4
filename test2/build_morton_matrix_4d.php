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



$max = 50;

for($w = 0; $w < $max; ++$w){
	for($z = 0; $z < $max; ++$z){
		for($y = 0; $y < $max; ++$y){
			for($x = 0; $x < $max; ++$x){
				$sub = "aaa.$x.$y.$z.$w";
				rawCommand($redis, "MC4ADD", "morton4d", $sub, $x, $y, $z, $w, $sub);

				$sub = "bbb.$x.$y.$z.$w";
				rawCommand($redis, "MC4ADD", "morton4d", $sub, $x, $y, $z, $w, $sub);
			}
		}
	}

	printf("%5d of %5d\r", $w, $max);
}

printf("done      \n");


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



$max = 1000;

for($y = 0; $y < $max; ++$y){
	for($x = 0; $x < $max; ++$x){
		$sub = "aaa.$x.$y";
		rawCommand($redis, "MC2ADD", "morton2d", $sub, $x, $y, $sub);

		$sub = "bbb.$x.$y";
		rawCommand($redis, "MC2ADD", "morton2d", $sub, $x, $y, $sub);
	}

	printf("%5d of %5d\r", $y, $max);
}

printf("done      \n");


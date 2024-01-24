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
	for($x = 0; $x < $max; ++$x)
		rawCommand($redis, "MC2SET", "morton", $x, $y, "x->$x,y->$y");

	printf("%5d of %5d\n", $y, $max);
}


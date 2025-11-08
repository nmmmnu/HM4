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



$result = rawCommand($redis, "IXMRANGEMULTI", "bulnews", " ", $argv[1], 20, "");

print_r($result);


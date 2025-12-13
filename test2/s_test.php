<?php
if (count($argv) < 2){
	printf("Add search parameters...\n");
	exit;
}

function rawCommand(){
	$args = func_get_args();

	$redis = $args[0];

	array_shift($args);

	// PHPRedis only
	return call_user_func_array( [ $redis, "rawCommand" ], $args );
}



$redis = new Redis();
$redis->connect("127.0.0.1");

$index		= "flagman_s";
$search		= $argv[1];
$count		= count($argv) > 2 ? $argv[2] : 10;
$from		= count($argv) > 3 ? $argv[3] : "";

if(1){
	echo	"Search : >>>$search<<<\n"	.
		"Count  : >>>$count<<<\n"	.
		"From   : >>>$from<<<\n"
	;
}

if(1){
	$result = rawCommand($redis, "IXSRANGE",	$index, $search, $count, $from);

	print_r($result);
}

if(0){
	$result = rawCommand($redis, "XNGETKEYS", "$index~$search", $count, "$index~$search");

	print_r($result);
}


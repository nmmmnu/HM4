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
//$index		= "f";
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
	$result = rawCommand($redis, "IXSRANGESTRICT",	$index, $search, $count, $from);

	print_r($result);
}

if(0){
	$result = rawCommand($redis, "IXSRANGEFLEX",	$index, $search, $count, $from);

	print_r($result);
}

if(0){
	$result = rawCommand($redis, "IXSGETINDEXES", "flagman_s", "0000368455");

	print_r($result);
}

if(0){
	$result = rawCommand($redis, "XNGETKEYS", "flagman_s~ели~9999636383~0000363617", $count, "flagman_s~ели~");

	print_r($result);
}



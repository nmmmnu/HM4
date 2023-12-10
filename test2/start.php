<?php

function expect($label, $bool){
	$ok = $bool ? " OK " : "FAIL";

	printf("%-30s [ %s ]\n", $label, $ok);

	if (!$bool)
		exit(1);
}

function pause($sec){
	printf("---please wait %d sec---\r", $sec);
	sleep($sec);
}

function rawCommand(){
	$args = func_get_args();

	$redis = $args[0];

	array_shift($args);

	// PHPRedis only
	return call_user_func_array( [ $redis, "rawCommand" ], $args );
}

function prefixCleanup_($redis, $prefix){
	$last = "$prefix";
	while ($last = rawCommand($redis, "xndel", $last, $prefix)){
	//	echo "last: $last\n";
	}
}



$redis = new Redis();
$redis->connect("127.0.0.1");


$dir = __DIR__ . "/tests/*.php";

foreach (glob($dir) as $filename){
	$cmd = "cmd_" . basename($filename, ".php");

	require_once $filename;

	$cmd($redis);
}


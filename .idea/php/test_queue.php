<?php
checkParameters();

$redis = new Redis();
$redis->connect("127.0.0.1");

process($redis, $argv[1], 1024 * 1024 * 4);



function process($redis, $key, $count){
	for($i = 0; $i < $count; ++$i){
		$redis->sadd($key, $i);
	}

	$errors = 0;

	for($i = 0; $i < $count; ++$i){
		$x = $redis->spop($key);

		if ($x != $i)
			++$errors;
	}

	printf("Errors: %d\n", $errors);
	printf("\tIf testing normal Redis, is OK to have errors.\n");
}

function checkParameters(){
	global $argv;

	if (count($argv) > 1)
		return;

	printf("HM4 :: Test Queue\n");
	printf("\n");
	printf("Usage:\n");
	printf("\t%s [set_control_key]\n", $argv[0]);

	exit;
}


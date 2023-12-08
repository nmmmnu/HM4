<?php

$redis = new Redis();
$redis->connect("127.0.0.1");

$count = 20 * 1024;



printf("Insert... (can take a while)\n");

for($i = 0; $i < $count; ++$i){
	$redis->set("a~$i", $i);

	testIntegrity($redis);
}

printf("Done %d items\n", $redis->dbsize());



printf("Checking...\n");

for($i = 0; $i < $count; ++$i)
	if ($redis->get("a~$i") != $i)
		printf(" - Error $i\n");

printf("Done %d items\n", $redis->dbsize());



printf("Deleting... (can take a while)\n");

for($i = 0; $i < $count; ++$i){
	$redis->del("a~$i");

	testIntegrity($redis);
}

printf("Done %d items\n", $redis->dbsize());



if ($redis->dbsize())
	printf(" - Error dbsize()\n");



function testIntegrity($redis){
	return rawCommand($redis, "test");
}

function rawCommand(){
	$args = func_get_args();

	$redis = $args[0];

	array_shift($args);

	// PHPRedis only
	return call_user_func_array( [ $redis, "rawCommand" ], $args );
}


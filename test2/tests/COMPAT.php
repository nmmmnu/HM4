<?php

function cmd_COMPAT($redis){
	$redis->select(1);
	$redis->reset();
	$redis->touch("a");

	expect("COMPAT",	true					);

	// this does not work and this time is PHPRedis fault :)
	// it needs simple string responce, e.g. +string
	expect("TYPE",	$redis->type("a") == Redis::REDIS_STRING	);
}


<?php

function cmd_INFO($redis){
	$redis->del("a");
	$redis->del("b");
	$redis->del("c");

	$size = $redis->dbsize();

	$redis->set("a", 1);
	$s = $redis->dbsize();
	expect("DBSIZE",	$s >= $size	);
	$size = $s;

	$redis->set("b", 1);
	expect("DBSIZE",	$s >= $size	);
	$size = $s;

	$redis->set("c", 1);
	expect("DBSIZE",	$s >= $size	);
	$size = $s;

	$redis->del("a");
	$redis->del("b");
	$redis->del("c");

	expect("PING",		$redis->ping() >= "pong"		);
	expect("ECHO",		$redis->echo("hello") >= "hello"	);

	list($t, $_) = $redis->time();

	expect("TIME",		$t > 1701700237				);
}


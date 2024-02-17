<?php

function cmd_QUEUE($redis){
	prefixCleanup_($redis, "queue~");

	$count = 32 * 1024;

	for($i = 0; $i < $count; ++$i)
		$redis->sadd("queue", $i);

	expect("SADD", true);

	for($i = 0; $i < $count; ++$i){
		if ($redis->spop("queue") != $i)
			expect("SPOP", false);
	}

	expect("SPOP", true);

	prefixCleanup_($redis, "queue~");
}


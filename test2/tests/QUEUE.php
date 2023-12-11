<?php

function cmd_QUEUE($redis){
	prefixCleanup_($redis, "a~");

	$count = 32 * 1024;

	for($i = 0; $i < $count; ++$i)
		$redis->sadd("a", $i);

	expect("SADD", true);

	for($i = 0; $i < $count; ++$i){
		if ($redis->spop("a") != $i)
			expect("SPOP", false);
	}

	expect("SPOP", true);

	prefixCleanup_($redis, "a~");
}


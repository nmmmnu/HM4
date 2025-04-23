<?php
$redis = new Redis();
$redis->connect("127.0.0.1");

$max = 1024 * 1024;

for($i = 0; $i < $max; ++$i)
	$redis->set(sprintf("key:%08d", $i), 1);


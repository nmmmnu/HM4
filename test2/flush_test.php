<?php

$redis = new Redis();
$redis->connect("127.0.0.1");

$big = getBig();

printf("%d\n", strlen($big));

$max = 1024;

for($i = 0; $i < $max; ++$i){
	$redis->set("$i:a", $i . $big);

	printf("%5d\n", $i);
}

//exit;

for($i = 0; $i < $max; ++$i){
	if ($redis->get("$i:a") != $i . $big){
		printf("Error on %5d\n", $i);
		break;
	}

	printf("%5d\n", $i);
}


function getBig(){
	$b = "";

	for($i = 0; $i < 1024; ++$i)
		$b = $b . "x";

	$a = "";

	for($i = 0; $i < 1024; ++$i)
		$a = $a . $b;

	return $a;
}


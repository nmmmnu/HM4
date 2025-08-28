<?php

$redis = new Redis();
$redis->connect("127.0.0.1");



$filename = "/home/nmmm/Development/load_fb_vector/wiki-news-300d-1M.vec";

$F = fopen($filename, "r");
if (!$F)
	die("Can not open $filename\n");



$header = fgets($F);
list($numWords, $dim) = explode(" ", trim($header));

$i = 0;

while (($line = fgets($F)) !== false){
	$parts = explode(" ", trim($line));

	$word = array_shift($parts);

	$vector = array_map("floatval", $parts);

	// if (strlen($word) < 3)
	// 	continue;

	process_vector($word, $vector);

	if (++$i % 25000 == 0)
		printf("Processed %10d...\n", $i);

	// if ($i >= 125)
	// 	break;
}

fclose($F);

printf("Loaded %d vectors\n", $i);



function process_vector($key, & $vector){
	global $redis;

	if (1){
		$x = $redis->rawCommand(
			"VADD",
			"gb150",
			300, 150, "b",
			"h", vhex($vector),
			$key
		);
	}

	if (0){
		$x = $redis->rawCommand(
			"VKSET",
			"gkb150:$key",
			300, 150, "b",
			"h", vhex($vector)
		);
	}

	if (0){
		$times = 14;

		$v2 = [];

		for ($i = 0; $i < $times; ++$i)
			$v2 = array_merge($v2, $vector);

		$x = $redis->rawCommand(
			"VADD",
			"b4200",
			4200, 4200, "b",
			"b", vbin($v2),
			$key
		);

		if ($key == "frog"){
			$x = $redis->rawCommand(
				"VKSET",
				"b4200_$key",
				4200, 4200, "f",
				"b", vbin($v2),
			);
		}

		unset($v2);
	}

}

function vbin(array & $vector){
	return pack("f*", ...$vector);
}

function vhex(array & $vector){
	return bin2hex(vbin($vector));
}



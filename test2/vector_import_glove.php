<?php
function rawCommand(){
	$args = func_get_args();

	$redis = $args[0];

	array_shift($args);

	// PHPRedis only
	return call_user_func_array( [ $redis, "rawCommand" ], $args );
}



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

	$x = $redis->rawCommand(
		"VKSET",
		"grawi150:$key",
		300, 150, "i",
		"b", vbin($vector)
	);
}

function vbin(array & $vector){
	return pack("f*", ...$vector);
}

function vhex(array & $vector){
	return bin2hex(vbin($vector));
}


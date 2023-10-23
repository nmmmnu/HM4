<?php
require_once dirname(__FILE__) . "/HM4Helper.php";



$redis = new Redis();
$redis->connect("127.0.0.1");
$redis_helper = new HM4Helper($redis);



$count	= 1;//0000;



switch(count($argv)){
case 2:		return processXU($redis_helper,           $argv[1], $count);
case 3:		return processXN($redis_helper, $argv[1], $argv[2], $count);
default:	return showHelp($argv[0]);
}



function processXU($redis_helper, $pattern, $count){
	$key = "";

	do{
		list($data, $key) = $redis_helper->xugetkeys($key, $count);

		process_($data, $pattern);
	}while($key);
}

function processXN($redis_helper, $prefix, $pattern, $count){
	$key = $prefix;

	do{
		list($data, $key) = $redis_helper->xngetkeys($key, $count, $prefix);

		process_($data, $pattern);
	}while($key);
}

function process_(array & $data, $pattern){
	foreach($data as $k => $v)
		if (fnmatch($pattern, $k))
			printf("%s\n", $k);
}



function showHelp($name){
	printf("HM4 :: keys\n");
	printf("\n");
	printf("Usage:\n");
	printf("\t%s          [pattern] - %s\n", $name, "display keys"					);
	printf("\t%s [prefix] [pattern] - %s\n", $name, "display keys with specific prefix (faster)"	);
}


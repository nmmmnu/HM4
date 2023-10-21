<?php
require_once dirname(__FILE__) . "/HM4Helper.php";



$redis = new Redis();
$redis->connect("127.0.0.1");
$redis_helper = new HM4Helper($redis);



$count	= 1;//0000;



switch(count($argv)){
case 2:		return process($redis_helper, false,    $argv[1], $count);
case 3:		return process($redis_helper, $argv[1], $argv[2], $count);
default:	return showHelp();
}



function process($redis_helper, $prefix, $pattern, $count){

	$f = function($key) use ($redis_helper, $prefix, $count){
		if ($prefix)
			return $redis_helper->xnget($key, $count, $prefix);
		else
			return $redis_helper->xuget($key, $count);
	};

	$key = $prefix ? $prefix : "";

	do{
		list($data, $key) = $f($key);

		foreach(array_keys($data) as $k)
			if (fnmatch($pattern, $k))
				printf("%s\n", $k);

	}while($key);
}

function showHelp(){
	printf("HM4 :: keys\n");
	printf("\n");
	printf("Usage:\n");
	printf("\t%s          [pattern]\n", $argv[0]);
	printf("\t%s [prefix] [pattern]\n", $argv[0]);
}


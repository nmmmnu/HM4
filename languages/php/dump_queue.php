<?php
require_once dirname(__FILE__) . "/RedisHelper.php";



function map($key, $val){
	printf("%s %s\n", "del", $key);
}





checkParameters();

$redis = new Redis();
$redis->connect("127.0.0.1");
$redis_helper = new AppBundle\AppBundle\Helper\RedisHelper($redis);

process($redis_helper, $argv[1]);



function process($redis_helper, $prefix){
	$key = $prefix;

	$i = 0;

	while($key){
		list($data, $key) = $redis_helper->getx($key, 10000, $prefix);

		foreach($data as $k => $v)
			map($k, $v);

		if (++$i % 100 == 0)
			fprintf(STDERR, "Processing: %10d %s\n", $i, $key);
	}
}

function checkParameters(){
	global $argv;

	if (count($argv) > 1)
		return;

	printf("HM4 :: dump_queue.php\n");
	printf("\n");
	printf("Usage:\n");
	printf("\t%s [set_control_key]\n", $argv[0]);

	exit;
}


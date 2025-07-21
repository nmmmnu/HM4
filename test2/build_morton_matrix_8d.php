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



$max = 8;

for($v00 = 0; $v00 < $max; ++$v00)
for($v01 = 0; $v01 < $max; ++$v01)
for($v02 = 0; $v02 < $max; ++$v02)
for($v03 = 0; $v03 < $max; ++$v03){	printf("%3d : %3d : %3d : %3d of %5d\r", $v00, $v01, $v02, $v03, $max);
for($v04 = 0; $v04 < $max; ++$v04)
for($v05 = 0; $v05 < $max; ++$v05)
for($v06 = 0; $v06 < $max; ++$v06)
for($v07 = 0; $v07 < $max; ++$v07){
		$sub = "aaa.$v00.$v01.$v02.$v03.$v04.$v05.$v06.$v07";

		rawCommand($redis, "MC8ADD", "morton8d", $sub,
					$v00, $v01, $v02, $v03, $v04, $v05, $v06, $v07,
							$sub);

		$sub = "bbb.$v00.$v01.$v02.$v03.$v04.$v05.$v06.$v07";

		rawCommand($redis, "MC8ADD", "morton8d", $sub,
					$v00, $v01, $v02, $v03, $v04, $v05, $v06, $v07,
							$sub);
	}
}



printf("done      \n");




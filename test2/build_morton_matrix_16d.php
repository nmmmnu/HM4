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



$max = 3;

for($v00 = 0; $v00 < $max; ++$v00)
for($v01 = 0; $v01 < $max; ++$v01)
for($v02 = 0; $v02 < $max; ++$v02)
for($v03 = 0; $v03 < $max; ++$v03)
for($v04 = 0; $v04 < $max; ++$v04)
for($v05 = 0; $v05 < $max; ++$v05){	printf("%3d : %3d : %3d : %3d : %3d : %3d of %5d\r", $v00, $v01, $v02, $v03, $v04, $v05, $max);
for($v06 = 0; $v06 < $max; ++$v06)
for($v07 = 0; $v07 < $max; ++$v07)
for($v08 = 0; $v08 < $max; ++$v08)
for($v09 = 0; $v09 < $max; ++$v09)
for($v10 = 0; $v10 < $max; ++$v10)
for($v11 = 0; $v11 < $max; ++$v11)
for($v12 = 0; $v12 < $max; ++$v12)
for($v13 = 0; $v13 < $max; ++$v13)
for($v14 = 0; $v14 < $max; ++$v14)
for($v15 = 0; $v15 < $max; ++$v15){
		if (rand(0, 100) > 10)
			continue;

		$sub = "aaa.$v00.$v01.$v02.$v03.$v04.$v05.$v06.$v07.$v08.$v09.$v10.$v11.$v12.$v13.$v14.$v15";

		rawCommand($redis, "MC16ADD", "morton16d", $sub,
					$v00, $v01, $v02, $v03, $v04, $v05, $v06, $v07, $v08, $v09, $v10, $v11, $v12, $v13, $v14, $v15,
							$sub);

		$sub = "bbb.$v00.$v01.$v02.$v03.$v04.$v05.$v06.$v07.$v08.$v09.$v10.$v11.$v12.$v13.$v14.$v15";

		rawCommand($redis, "MC16ADD", "morton16d", $sub,
					$v00, $v01, $v02, $v03, $v04, $v05, $v06, $v07, $v08, $v09, $v10, $v11, $v12, $v13, $v14, $v15,
							$sub);
	}
}



printf("done      \n");




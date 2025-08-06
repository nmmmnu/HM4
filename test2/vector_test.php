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



// clean up.
rawCommand($redis, "xndel", "wf", "wf");
rawCommand($redis, "xndel", "wi", "wi");



// add restaurants in Sofia
$x = $redis->rawCommand(
	"VADD",
	"wf",
	2, "F", "b",
	vbin( [ 0, 0 ] ), "w00",
	vbin( [ 0, 1 ] ), "w01",
	vbin( [ 1, 0 ] ), "w10",
	vbin( [ 1, 1 ] ), "w11",
	vbin( [ 1, 2 ] ), "w12",
	vbin( [ 2, 1 ] ), "w21",
	vbin( [ 2, 2 ] ), "w22"
);

var_dump($x);



$x = $redis->rawCommand(
	"VADD",
	"wi",
	2, "I", "h",
	vhex( [ 0, 0 ] ), "w00",
	vhex( [ 0, 1 ] ), "w01",
	vhex( [ 1, 0 ] ), "w10",
	vhex( [ 1, 1 ] ), "w11",
	vhex( [ 1, 2 ] ), "w12",
	vhex( [ 2, 1 ] ), "w21",
	vhex( [ 2, 2 ] ), "w22"
);

var_dump($x);

//echo "VADD x 2 i h " . vhex( [ 1, 2 ] ) . " t22";



function vbin(array & $vector){
	return pack("f*", ...$vector);
}

function vhex(array & $vector){
	return bin2hex(vbin($vector));
}




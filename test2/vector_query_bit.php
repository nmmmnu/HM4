<?php

$redis = new Redis();
$redis->connect("127.0.0.1");

$raw		= "b";
$results	= 10;
$vsim		= "VSIMFLAT";
//$vsim		= "VSIMLSH";
$distBit	= "b";
$distBit	= "h";

$frog = $redis->rawCommand(
	"VKGETRAW"	,
	"b4200_frog"	,
	4200		,
	"f"		,
	$raw
);

// echo "$frog";


$result = $redis->rawCommand(
	$vsim		,
	"b4200"		,
	4200		,
	4200		,
	"b"		,
	$distBit	,
	$raw		,
	$frog		,
	$results
);

print_r($result);


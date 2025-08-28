<?php

$redis = new Redis();
$redis->connect("127.0.0.1");

$raw		= "b";
$results	= 10;
$vsim		= "VSIMFLAT";
$vsim		= "VSIMLSH";

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
	"h"		,
	$raw		,
	$frog		,
	$results
);

print_r($result);


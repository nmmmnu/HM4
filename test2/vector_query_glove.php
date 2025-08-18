<?php

$redis = new Redis();
$redis->connect("127.0.0.1");

$raw		= "b";
$results	= 10;
$vsim		= "VSIMFLAT";
$dist		= "k";


$frog = $redis->rawCommand(
	"VGETRAW"	,
	"gf300"		,
	300		,
	"f"		,
	$raw		,
	"frog"
);

// echo "$frog";



$result = $redis->rawCommand(
	$vsim		,
	"gf300"		,
	300		,
	300		,
	"f"		,
	$dist		,
	$raw		,
	$frog		,
	$results
);

echo "300 300 float\n";
print_r($result);



$result = $redis->rawCommand(
	$vsim		,
	"gf150"		,
	300		,
	150		,
	"f"		,
	$dist		,
	$raw		,
	$frog		,
	$results
);

echo "300 150 float\n";
print_r($result);



$result = $redis->rawCommand(
	$vsim		,
	"gs300"		,
	300		,
	300		,
	"s"		,
	$dist		,
	$raw		,
	$frog		,
	$results
);

echo "300 300 short\n";
print_r($result);



$result = $redis->rawCommand(
	$vsim		,
	"gs150"		,
	300		,
	150		,
	"s"		,
	$dist		,
	$raw		,
	$frog		,
	$results
);

echo "300 150 short\n";
print_r($result);



$result = $redis->rawCommand(
	$vsim		,
	"gi300"		,
	300		,
	300		,
	"i"		,
	$dist		,
	$raw		,
	$frog		,
	$results
);

echo "300 300 i8\n";
print_r($result);



$result = $redis->rawCommand(
	$vsim		,
	"gi150"		,
	300		,
	150		,
	"i"		,
	$dist		,
	$raw		,
	$frog		,
	$results
);

echo "300 150 i8\n";
print_r($result);



$result = $redis->rawCommand(
	$vsim		,
	"gb300"		,
	300		,
	300		,
	"b"		,
	"h"		,
	$raw		,
	$frog		,
	$results
);

echo "300 300 bit\n";
print_r($result);



$result = $redis->rawCommand(
	$vsim		,
	"gb150"		,
	300		,
	150		,
	"b"		,
	"h"		,
	$raw		,
	$frog		,
	$results
);

echo "300 150 bit\n";
print_r($result);



$frog = $redis->rawCommand(
	"VGETRAW"	,
	"gf150"		,
	150		,
	"f"		,
	$raw		,
	"frog"
);

//echo "$frog";



$result = $redis->rawCommand(
	$vsim		,
	"gf150"		,
	150		,
	150		,
	"f"		,
	$dist		,
	$raw		,
	$frog		,
	$results
);

echo "150 150 float\n";
print_r($result);



$result = $redis->rawCommand(
	$vsim		,
	"gs150"		,
	150		,
	150		,
	"s"		,
	$dist		,
	$raw		,
	$frog		,
	$results
);

echo "150 150 short\n";
print_r($result);



$result = $redis->rawCommand(
	$vsim		,
	"gi150"		,
	150		,
	150		,
	"i"		,
	$dist		,
	$raw		,
	$frog		,
	$results
);

echo "150 150 i8\n";
print_r($result);



$result = $redis->rawCommand(
	$vsim		,
	"gb150"		,
	150		,
	150		,
	"b"		,
	"h"		,
	$raw		,
	$frog		,
	$results
);

echo "150 150 bit\n";
print_r($result);






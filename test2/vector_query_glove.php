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

echo "300 300 f\n";
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

echo "300 150 f\n";
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

echo "300 300 s\n";
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

echo "300 150 s\n";
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

echo "300 300 i\n";
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

echo "300 150 i\n";
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

echo "150 150 f\n";
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

echo "150 150 s\n";
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

echo "150 150 i\n";
print_r($result);






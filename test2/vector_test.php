<?php

$redis = new Redis();
$redis->connect("127.0.0.1");

// clean up.
$redis->rawCommand("xndel", "vectors", "vectors");

$x = $redis->rawCommand(
	"VADD",
	"vectors",
	3,
	3,
	"f",
	"b",
	vbin_le( [ 0, 0, 0 ] ), "w000",
	vbin_le( [ 0, 0, 1 ] ), "w001",
	vbin_le( [ 0, 1, 0 ] ), "w010",
	vbin_le( [ 0, 1, 1 ] ), "w011",
	vbin_le( [ 0, 1, 2 ] ), "w012",
	vbin_le( [ 0, 2, 1 ] ), "w021",
	vbin_le( [ 0, 2, 2 ] ), "w022"
);

var_dump($x);

$x = $redis->rawCommand(
	"VADD",
	"vectors",
	3,
	3,
	"f",
	"B",
	vbin_be( [ 1, 0, 0 ] ), "w100",
	vbin_be( [ 1, 0, 1 ] ), "w101",
	vbin_be( [ 1, 1, 0 ] ), "w110",
	vbin_be( [ 1, 1, 1 ] ), "w111",
	vbin_be( [ 1, 1, 2 ] ), "w112",
	vbin_be( [ 1, 2, 1 ] ), "w121",
	vbin_be( [ 1, 2, 2 ] ), "w122"
);

var_dump($x);

$x = $redis->rawCommand(
	"VADD",
	"vectors",
	3,
	3,
	"f",
	"h",
	vhex_le( [ 2, 0, 0 ] ), "w200",
	vhex_le( [ 2, 0, 1 ] ), "w201",
	vhex_le( [ 2, 1, 0 ] ), "w210",
	vhex_le( [ 2, 1, 1 ] ), "w211",
	vhex_le( [ 2, 1, 2 ] ), "w212",
	vhex_le( [ 2, 2, 1 ] ), "w221",
	vhex_le( [ 2, 2, 2 ] ), "w222"
);

var_dump($x);

$x = $redis->rawCommand(
	"VADD",
	"vectors",
	3,
	3,
	"f",
	"H",
	vhex_be( [ 3, 0, 0 ] ), "w300",
	vhex_be( [ 3, 0, 1 ] ), "w301",
	vhex_be( [ 3, 1, 0 ] ), "w310",
	vhex_be( [ 3, 1, 1 ] ), "w311",
	vhex_be( [ 3, 1, 2 ] ), "w312",
	vhex_be( [ 3, 2, 1 ] ), "w321",
	vhex_be( [ 3, 2, 2 ] ), "w322"
);

var_dump($x);

print_r(
	$redis->rawCommand(
		"VGET",
		"vectors",
		3,
		"f",
		"w021"
	)
);
print_r(
	$redis->rawCommand(
		"VGET",
		"vectors",
		3,
		"f",
		"w121"
	)
);
print_r(
	$redis->rawCommand(
		"VGET",
		"vectors",
		3,
		"f",
		"w221"
	)
);
print_r(
	$redis->rawCommand(
		"VGET",
		"vectors",
		3,
		"f",
		"w321"
	)
);

function vbin(array $vector){
	// f = float, host order (x86, arm = little endian)
	return pack("f*", ...$vector);
}

function vbin_le(array $vector){
	// g = float, little endian
	return pack("g*", ...$vector);
}

function vbin_be(array $vector){
	// G = float, big endian
	return pack("G*", ...$vector);
}



function vhex(array $vector){
	return bin2hex(vbin($vector));
}

function vhex_le(array $vector){
	return bin2hex(vbin_le($vector));
}

function vhex_be(array $vector){
	return bin2hex(vbin_be($vector));
}



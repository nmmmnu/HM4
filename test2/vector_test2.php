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
	vbin_le( [ 0, 0, 1 ] ), "le"
);

$x = $redis->rawCommand(
	"VADD",
	"vectors",
	3,
	3,
	"f",
	"B",
	vbin_be( [ 0, 0, 1 ] ), "be"
);
/*
$x = $redis->rawCommand(
	"VADD",
	"vectors",
	3,
	3,
	"f",
	"h",
	vhex_le( [ 0, 0, +5.0207 ] ), "le"
);

$x = $redis->rawCommand(
	"VADD",
	"vectors",
	3,
	3,
	"f",
	"h",
	vhex_be( [ 0, 0, -5.5156 ] ), "be"
);
*/
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



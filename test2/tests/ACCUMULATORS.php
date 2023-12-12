<?php
require_once("sample_data.php");

function cmd_ACCUMULATORS_testXN($redis, $cmd, $key, $prefix, $result){
	$x = rawCommand($redis, $cmd, $key, $prefix);

	expect("$cmd count",	count($x) == 2				);
	expect("$cmd result",	$x[0] == $result			);
	expect("$cmd next",	$x[1] == ''				);
}

function cmd_ACCUMULATORS_testXR($redis, $cmd, $key, $prefix, $result){
	$x = rawCommand($redis, $cmd, $key, $prefix);

	print_r($x);

	expect("$cmd count",	count($x) == 2				);
	expect("$cmd result",	$x[0] == $result			);
	expect("$cmd next",	$x[1] == ''				);
}

function cmd_ACCUMULATORS_testCC($redis, $cmd, $key, $prefix, $result){
	$x = rawCommand($redis, $cmd, $key, 10000, $prefix);

	expect("$cmd count",	count($x) == 2				);
	expect("$cmd result",	$x[0] == $result			);
	expect("$cmd next",	$x[1] == ''				);
}

function cmd_ACCUMULATORS($redis){
	sampleData_insert($redis);

	cmd_ACCUMULATORS_testCC($redis, "COUNT",	"a~1984",	"a~",		40	);
	cmd_ACCUMULATORS_testCC($redis, "SUM",		"a~1984",	"a~",		32380	);

	cmd_ACCUMULATORS_testXN($redis, "XNCOUNT",	"a~1984",	"a~",		40	);
	cmd_ACCUMULATORS_testXN($redis, "XNSUM",	"a~1984",	"a~",		32380	);
	cmd_ACCUMULATORS_testXN($redis, "XNMIN",	"a~1984",	"a~",		272	);	// year 2000
	cmd_ACCUMULATORS_testXN($redis, "XNMAX",	"a~1984",	"a~",		1984	);	// year 2023
	cmd_ACCUMULATORS_testXN($redis, "XNFIRST",	"a~1984",	"a~",		309	);	// year 1984
	cmd_ACCUMULATORS_testXN($redis, "XNLAST",	"a~1984",	"a~",		1984	);	// year 2023
	cmd_ACCUMULATORS_testXN($redis, "XNAVG",	"a~1984",	"a~",		809	);

	cmd_ACCUMULATORS_testXR($redis, "XRCOUNT",	"a~1984",	"a~2023",	40	);
	cmd_ACCUMULATORS_testXR($redis, "XRSUM",	"a~1984",	"a~2023",	32380	);
	cmd_ACCUMULATORS_testXR($redis, "XRMIN",	"a~1984",	"a~2023",	272	);	// year 2000
	cmd_ACCUMULATORS_testXR($redis, "XRMAX",	"a~1984",	"a~2023",	1984	);	// year 2023
	cmd_ACCUMULATORS_testXR($redis, "XRFIRST",	"a~1984",	"a~2023",	309	);	// year 1984
	cmd_ACCUMULATORS_testXR($redis, "XRLAST",	"a~1984",	"a~2023",	1984	);	// year 2023
	cmd_ACCUMULATORS_testXR($redis, "XRAVG",	"a~1984",	"a~2023",	809	);

	sampleData_delete($redis);
}


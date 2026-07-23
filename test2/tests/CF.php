<?php

function cmd_CF($redis){
	$redis->del("a");

	foreach([8, 16, 32] as $fingerprint_size){
		$width = 1024;

		expect("CFEXISTS",	rawCommand($redis, "cfexists",	"a", $width, $fingerprint_size, "item1") == 0);

		expect("CFRESERVE",	rawCommand($redis, "cfreserve",	"a", $width, $fingerprint_size) == 1);

		expect("CFADD",		rawCommand($redis, "cfadd",	"a", $width, $fingerprint_size, "item1") == 1);
		expect("CFMADD",	rawCommand($redis, "cfmadd",	"a", $width, $fingerprint_size, "item2", "item3", "item4") == [1, 1, 1] );

		expect("CFEXISTS",	rawCommand($redis, "cfexists",	"a", $width, $fingerprint_size, "item1") == 1);
		expect("CFEXISTS",	rawCommand($redis, "cfexists",	"a", $width, $fingerprint_size, "item2") == 1);
		expect("CFEXISTS",	rawCommand($redis, "cfexists",	"a", $width, $fingerprint_size, "non_existent") == 0);

		expect("CFMEXISTS",	rawCommand($redis, "cfmexists",	"a", $width, $fingerprint_size, "item1", "non_existent", "item3") == [1, 0, 1]);

		expect("CFREM",		rawCommand($redis, "cfrem",	"a", $width, $fingerprint_size, "item1") == 1);
		expect("CFEXISTS",	rawCommand($redis, "cfexists",	"a", $width, $fingerprint_size, "item1") == 0);

		expect("CFMREM",	rawCommand($redis, "cfmrem",	"a", $width, $fingerprint_size, "item2", "item3") == [1, 1]);

		expect("CFMEXISTS",	rawCommand($redis, "cfmexists",	"a", $width, $fingerprint_size, "item2", "item3") == [0, 0]);

		expect("CFREM",		rawCommand($redis, "cfrem",	"a", $width, $fingerprint_size, "item1") == 0);

		$redis->del("a");
	}
}



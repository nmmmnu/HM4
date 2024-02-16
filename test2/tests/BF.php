<?php

function cmd_BF($redis){
	$redis->del("a");
	$redis->del("b");

	$bits = 4096;
	$hf   = 4;

	rawCommand($redis, "BFRESERVE", "a", $bits, $hf);

	rawCommand($redis, "BFADD", "a", $bits, $hf, "Sofia");
	rawCommand($redis, "BFADD", "a", $bits, $hf, "Varna");
	rawCommand($redis, "BFADD", "a", $bits, $hf, "New York", "London", "Kiev", "Los Angeles");

	expect("BFADD",		true				);

	rawCommand($redis, "BFRESERVE", "a", $bits, $hf);

	expect("BFEXISTS",	rawCommand($redis, "BFEXISTS", "a", $bits, $hf, "Sofia") == true	);
	expect("BFEXISTS",	rawCommand($redis, "BFEXISTS", "a", $bits, $hf, "Rome" ) == false	);
	expect("BFEXISTS",	rawCommand($redis, "BFEXISTS", "b", $bits, $hf, "Rome" ) == false	);

	$m  = rawCommand($redis, "BFMEXISTS", "a", $bits, $hf, "New York", "London", "Rome");

	expect("BFMEXISTS",	$m[0] && $m[1] && !$m[2]	);

	$m  = rawCommand($redis, "BFMEXISTS", "b", $bits, $hf, "New York", "London", "Rome");

	expect("BFMEXISTS",	!$m[0] && !$m[1] && !$m[2]	);

	$redis->del("a");
}


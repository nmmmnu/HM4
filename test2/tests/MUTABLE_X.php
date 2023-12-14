<?php

function cmd_MUTABLE_X($redis){
	if (true){
		if (true){
			sampleData_insert($redis);

			$x = rawCommand($redis, "xnget", "a~1974", 10, "a~");
			expect("XNGET ",	count($x) == 21				);

			$last = "a~";
			while ($last = rawCommand($redis, "xndel", $last, "a~")){
			}

			$x = rawCommand($redis, "xnget", "a~1974", 10, "a~");
			expect("XNDEL",		count($x) == 1 && $x[0] == ""		);
		}

		if (true){
			sampleData_insert($redis);

			$x = rawCommand($redis, "xrget", "a~1974", 10, "a~2022");
			expect("XRGET count",	count($x) == 21				);

			$last = "a~";
			while ($last = rawCommand($redis, "xrdel", $last, "a~2022")){
			}

			$x = rawCommand($redis, "xrget", "a~1974", 10, "a~2022");
			expect("XRDEL",		count($x) == 1 && $x[0] == ""		);

			expect("XRDEL last+1",	$redis->exists("a~2023")		);
		}
	}

	if (true){
		if (true){
			sampleData_insert($redis);

			$x = rawCommand($redis, "xnget", "a~1974", 10, "a~");
			expect("XNGET ",	count($x) == 21				);

			$last = "a~";
			while ($last = rawCommand($redis, "xnexpire", $last, 1, "a~")){
			}

			$x = rawCommand($redis, "xnget", "a~1974", 10, "a~");
			expect("XNEXPIRE ",	count($x) == 21				);

			pause(2);

			$x = rawCommand($redis, "xnget", "a~1974", 10, "a~");
			expect("XNEXPIRE ",	count($x) == 1				);
		}

		if (true){
			sampleData_insert($redis);

			$x = rawCommand($redis, "xrget", "a~1974", 10, "a~2022");
			expect("XRGET count",	count($x) == 21				);

			$last = "a~";
			while ($last = rawCommand($redis, "xrexpire", $last, 1, "a~2022")){
			}

			$x = rawCommand($redis, "xrget", "a~1974", 10, "a~2022");
			expect("XRGET count",	count($x) == 21				);

			pause(2);

			$x = rawCommand($redis, "xrget", "a~1974", 10, "a~2022");
			expect("XRDEL",		count($x) == 1 && $x[0] == ""		);

			expect("XRDEL last+1",	$redis->exists("a~2023")		);
		}
	}

	if (true){
		if (true){
			sampleData_insert($redis);

			$x = rawCommand($redis, "xnget", "a~1974", 10, "a~");
			expect("XNGET ",	count($x) == 21				);

			$last = "a~";
			while ($last = rawCommand($redis, "xnexpire", $last, 1, "a~")){
			}

			$last = "a~";
			while ($last = rawCommand($redis, "xnpersist", $last, "a~")){
			}

			$x = rawCommand($redis, "xnget", "a~1974", 10, "a~");
			expect("XNPERSIST ",	count($x) == 21				);

			expect("XNPERSIST ttl",	$redis->ttl("a~1974") == 0		);
			expect("XNPERSIST ttl",	$redis->ttl("a~2022") == 0		);
			expect("XNPERSIST ttl",	$redis->ttl("a~2023") == 0		);
		}

		if (true){
			sampleData_insert($redis);

			$x = rawCommand($redis, "xrget", "a~1974", 10, "a~2022");
			expect("XRGET count",	count($x) == 21				);

			$last = "a~";
			while ($last = rawCommand($redis, "xrexpire", $last, 1, "a~2022")){
			}

			$last = "a~";
			while ($last = rawCommand($redis, "xrpersist", $last, "a~2022")){
			}

			$x = rawCommand($redis, "xrget", "a~1974", 10, "a~2022");
			expect("XRGET count",	count($x) == 21				);

			expect("XNPERSIST ttl",	$redis->ttl("a~1974") == 0		);
			expect("XNPERSIST ttl",	$redis->ttl("a~2022") == 0		);
			expect("XNPERSIST ttl",	$redis->ttl("a~2023") == 0		);
		}
	}

	sampleData_delete($redis);
}


<?php
require_once("sample_data.php");

function cmd_IMMUTABLE_X($redis){
	sampleData_insert($redis);

	if (true){
		if (true){
			$x = rawCommand($redis, "xuget", "a~1974", 10);

			expect("XUGET count",	count($x) == 21				);
			expect("XUGET end",	end($x)   == "a~1984"			);
			expect("XUGET data",	$x[0] == "a~1974" && $x[1] == 187	);
			expect("XUGET data",	$x[2] == "a~1975" && $x[3] == 140	);

			$x = rawCommand($redis, "xuget", end($x), 10);

			expect("XUGET count",	count($x) == 21				);
			expect("XUGET end",	end($x)   == "a~1994"			);
			expect("XUGET data",	$x[0] == "a~1984" && $x[1] == 309	);
			expect("XUGET data",	$x[2] == "a~1985" && $x[3] == 327	);
		}

		if (true){
			$x = rawCommand($redis, "xugetkeys", "a~1974", 10);

			expect("XUGETKEYS count",	count($x) == 21			);
			expect("XUGETKEYS end",		end($x)   == "a~1984"		);
			expect("XUGETKEYS data",	$x[0] == "a~1974" && $x[1] == 1	);
			expect("XUGETKEYS data",	$x[2] == "a~1975" && $x[3] == 1	);

			$x = rawCommand($redis, "xugetkeys", end($x), 10);

			expect("XUGETKEYS count",	count($x) == 21			);
			expect("XUGETKEYS end",		end($x)   == "a~1994"		);
			expect("XUGETKEYS data",	$x[0] == "a~1984" && $x[1] == 1	);
			expect("XUGETKEYS data",	$x[2] == "a~1985" && $x[3] == 1	);
		}
	}

	if (true){
		if (true){
			$x = rawCommand($redis, "xnget", "a~1974", 10, "a~");

			expect("XNGET count",	count($x) == 21				);
			expect("XNGET end",	end($x)   == "a~1984"			);
			expect("XNGET data",	$x[0] == "a~1974" && $x[1] == 187	);
			expect("XNGET data",	$x[2] == "a~1975" && $x[3] == 140	);

			$x = rawCommand($redis, "xnget", end($x), 10, "a~");

			expect("XNGET count",	count($x) == 21				);
			expect("XNGET end",	end($x)   == "a~1994"			);
			expect("XNGET data",	$x[0] == "a~1984" && $x[1] == 309	);
			expect("XNGET data",	$x[2] == "a~1985" && $x[3] == 327	);

			$x = rawCommand($redis, "xnget", "a~2020", 10, "a~");

			expect("XNGET count",	count($x) == 9				);
			expect("XNGET end",	end($x)   == ""				);
			expect("XNGET data",	$x[0] == "a~2020" && $x[1] == 1895	);
			expect("XNGET data",	$x[2] == "a~2021" && $x[3] == 1828	);

		}

		if (true){
			$x = rawCommand($redis, "xngetkeys", "a~1974", 10, "a~");

			expect("XNGETKEYS count",	count($x) == 21			);
			expect("XNGETKEYS end",		end($x)   == "a~1984"		);
			expect("XNGETKEYS data",	$x[0] == "a~1974" && $x[1] == 1	);
			expect("XNGETKEYS data",	$x[2] == "a~1975" && $x[3] == 1	);

			$x = rawCommand($redis, "xngetkeys", end($x), 10, "a~");

			expect("XNGETKEYS count",	count($x) == 21			);
			expect("XNGETKEYS end",		end($x)   == "a~1994"		);
			expect("XNGETKEYS data",	$x[0] == "a~1984" && $x[1] == 1	);
			expect("XNGETKEYS data",	$x[2] == "a~1985" && $x[3] == 1	);

			$x = rawCommand($redis, "xngetkeys", "a~2020", 10, "a~");

			expect("XNGETKEYS count",	count($x) == 9			);
			expect("XNGETKEYS end",		end($x)   == ""			);
			expect("XNGETKEYS data",	$x[0] == "a~2020" && $x[1] == 1	);
			expect("XNGETKEYS data",	$x[2] == "a~2021" && $x[3] == 1	);

		}
	}

	if (true){
		if (true){
			$x = rawCommand($redis, "xrget", "a~1974", 10, "a~2023");

			expect("XRGET count",	count($x) == 21				);
			expect("XRGET end",	end($x)   == "a~1984"			);
			expect("XRGET data",	$x[0] == "a~1974" && $x[1] == 187	);
			expect("XRGET data",	$x[2] == "a~1975" && $x[3] == 140	);

			$x = rawCommand($redis, "xrget", end($x), 10, "a~2023");

			expect("XRGET count",	count($x) == 21				);
			expect("XRGET end",	end($x)   == "a~1994"			);
			expect("XRGET data",	$x[0] == "a~1984" && $x[1] == 309	);
			expect("XRGET data",	$x[2] == "a~1985" && $x[3] == 327	);

			$x = rawCommand($redis, "xrget", "a~2020", 10, "a~2023");

			expect("XRGET count",	count($x) == 9				);
			expect("XRGET end",	end($x)   == ""				);
			expect("XRGET data",	$x[0] == "a~2020" && $x[1] == 1895	);
			expect("XRGET data",	$x[2] == "a~2021" && $x[3] == 1828	);
		}

		if (true){
			$x = rawCommand($redis, "xrgetkeys", "a~1974", 10, "a~2023");

			expect("XRGETKEYS count",	count($x) == 21			);
			expect("XRGETKEYS end",		end($x)   == "a~1984"		);
			expect("XRGETKEYS data",	$x[0] == "a~1974" && $x[1] == 1	);
			expect("XRGETKEYS data",	$x[2] == "a~1975" && $x[3] == 1	);

			$x = rawCommand($redis, "xrgetkeys", end($x), 10, "a~2023");

			expect("XRGETKEYS count",	count($x) == 21			);
			expect("XRGETKEYS end",		end($x)   == "a~1994"		);
			expect("XRGETKEYS data",	$x[0] == "a~1984" && $x[1] == 1	);
			expect("XRGETKEYS data",	$x[2] == "a~1985" && $x[3] == 1	);

			$x = rawCommand($redis, "xrgetkeys", "a~2020", 10, "a~2023");

			expect("XRGETKEYS count",	count($x) == 9			);
			expect("XRGETKEYS end",		end($x)   == ""			);
			expect("XRGETKEYS data",	$x[0] == "a~2020" && $x[1] == 1	);
			expect("XRGETKEYS data",	$x[2] == "a~2021" && $x[3] == 1	);
		}
	}

	sampleData_delete($redis);
}


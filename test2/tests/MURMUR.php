<?php

function cmd_MURMUR($redis){
	expect("MURMUR",	rawCommand($redis, "murmur", "hello"		) == 2191231550387646743	);
	expect("MURMUR",	rawCommand($redis, "murmur", "hello",	0	) == 2191231550387646743	);
	expect("MURMUR",	rawCommand($redis, "murmur", "hello",	0, 16	) == 2191231550387646743 % 16	);
	expect("MURMUR",	rawCommand($redis, "murmur", "hello",	1	) == 5983625672228268878	);
	expect("MURMUR",	rawCommand($redis, "murmur", "hello",	1, 16	) == 5983625672228268878 % 16	);
	expect("MURMUR",	rawCommand($redis, "murmur", "hello",	1, 16	) == 5983625672228268878 % 16	);
}


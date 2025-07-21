<?php
return array(
	new Cmd(
			"MC4GET",

			"MC4GET key subkey",

			"Get value of the <i>subkey</i> stored in <i>key</i>.",
			"string",
			"value of the subkey",
			"1.3.10.2",
			"2 * READ",
			false,
			false,

			"mortoncurve4d"
	),

	new Cmd(
			"MC4MGET",

			"MC4MGET key subkey [subkey]...",

			"Get values of the <i>subkey</i> stored in <i>key</i>.",
			"array",
			"values of the items",
			"1.3.10.2",
			"[number of items] * 2 * READ",
			false,
			false,

			"mortoncurve4d"
	),

	new Cmd(
			"MC4EXISTS",

			"MC4EXISTS key subkey",

			"Check if <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"0 if the item do not exists.<br />" .
			"1 if the item exists.",
			"1.3.10.2",
			"2 * READ",
			false,
			false,

			"mortoncurve4d"
	),

	new Cmd(
			"MC4SCORE",

			"MC4SCORE key subkey",

			"Get score of the <i>subkey</i> stored in <i>key</i>.",
			"array",
			"x, y and z",
			"1.3.10.2",
			"READ",
			false,
			false,

			"mortoncurve4d"
	),

	new Cmd(
			"MC4ADD",

			"MC4ADD key subKey x y z w value [subKey x y z w value]...",

			"Set <i>subkey</i> with 4D coordinates <i>x</i>, <i>y</i>, <i>z</i>, <i>w</i> and <i>value</i> in <i>key</i>.<br />" .
			"Uses <b>uint32_t</b> for coordinates.",
			"OK",
			"OK",
			"1.3.10.2",
			"[number of items] * (READ + 3 * WRITE), TX",
			false,
			true,

			"mortoncurve4d"
	),

	new Cmd(
			"MC4REM",

			"MC4REM key subkey [subkey]...",

			"Removes <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"Always return 1",
			"1.3.10.2",
			"[number of items] * (READ + 2 * WRITE), TX",
			false,
			true,

			"mortoncurve4d"
	),

	new Cmd(
			"MC4POINT",

			"MC4POINT key x y z w number [start]",

			"Gets all subkeys stored in 'point' with 4D coordinates <i>x</i>, <i>y</i> <i>z</i> and <i>w</i>.<br />" .
			"Uses <b>uint32_t</b> for coordinates.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"Return up to <i>number</i> of pairs.<br />" .
			"Returns up to ~32'000 elements.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.10.2",
			"READ",
			false,
			false,

			"mortoncurve4d"
	),

	new Cmd(
			"MC4RANGE",

			"MC4RANGE key x_min x_max y_min y_max z_min z_max w_min w_max number [start]",

			"Gets all subkeys stored in 'rectangle' with 4D coordinates <i>x_min</i> and <i>y_min</i> to <i>x_max</i> and <i>y_max</i> to <i>z_max</i> and <i>z_max</i> to <i>w_max</i> and <i>w_max</i>.<br />" .
			"Uses <b>uint32_t</b> for coordinates.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"Return up to <i>number</i> of pairs.<br />" .
			"Returns up to ~32'000 elements.<br />" .
			"Note this command is useful only on very large datasets.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.10.2",
			"N * READ, were N &isin; { 1 .. 9 }",
			false,
			false,

			"mortoncurve4d"
	),

	new Cmd(
			"MC4RANGENAIVE",

			"MC4RANGENAIVE key x_min x_max y_min y_max z_min z_max number [start]",

			"Naive, non performant version of MC4XNGET. Made for small datasets and for testing.<br />" .
			"See MC4RANGE for more information.<br />" .
			"Uses <b>uint32_t</b> for coordinates.<br />" .
			"Technically the data lookup complexity is single READ, but on large datasets, it might need to read complete dataset.",
			"array",
			"results",
			"1.3.10.2",
			"READ",
			false,
			false,

			"mortoncurve4d"
	),

	new Cmd(
			"MC4ENCODE",

			"MC4ENCODE x y z w",

			"Encode <i>x</i>, <i>y</i> and <i>z</i> to morton code hex string.<br />" .
			"Uses <b>uint32_t</b> for coordinates.",
			"string",
			"morton code hex string",
			"1.3.10.2",
			"READ",
			false,
			null,

			"mortoncurve4d"
	),

	new Cmd(
			"MC4DECODE",

			"MC4DECODE hex",

			"Decode <i>morton code hex string</i> to x, y, z, w.",
			"array",
			"x, y, z and w",
			"1.3.10.2",
			"READ",
			false,
			null,

			"mortoncurve4d"
	),

);

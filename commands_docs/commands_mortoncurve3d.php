<?php
return array(
	new Cmd(
			"MC3GET",

			"MC3GET key subkey",

			"Get value of the <i>subkey</i> stored in <i>key</i>.",
			"string",
			"value of the subkey",
			"1.3.10.2",
			"2 * READ",
			false,
			false,

			"mortoncurve3d"
	),

	new Cmd(
			"MC3MGET",

			"MC3MGET key subkey [subkey]...",

			"Get values of the <i>subkey</i> stored in <i>key</i>.",
			"array",
			"values of the items",
			"1.3.10.2",
			"[number of items] * 2 * READ",
			false,
			false,

			"mortoncurve3d"
	),

	new Cmd(
			"MC3EXISTS",

			"MC3EXISTS key subkey",

			"Check if <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"0 if the item do not exists.<br />" .
			"1 if the item exists.",
			"1.3.10.2",
			"2 * READ",
			false,
			false,

			"mortoncurve3d"
	),

	new Cmd(
			"MC3SCORE",

			"MC3SCORE key subkey",

			"Get score of the <i>subkey</i> stored in <i>key</i>.",
			"array",
			"x, y and z",
			"1.3.10.2",
			"READ",
			false,
			false,

			"mortoncurve3d"
	),

	new Cmd(
			"MC3ADD",

			"MC3ADD key subKey x y z value [subKey x y z value]...",

			"Set <i>subkey</i> with 3D coordinates <i>x</i>, <i>y</i>, <i>z</i> and <i>value</i> in <i>key</i>.<br />" .
			"Uses <b>uint32_t</b> for coordinates.",
			"OK",
			"OK",
			"1.3.10.2",
			"[number of items] * (READ + 3 * WRITE), TX",
			false,
			true,

			"mortoncurve3d"
	),

	new Cmd(
			"MC3REM",

			"MC3REM key subkey [subkey]...",

			"Removes <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"Always return 1",
			"1.3.10.2",
			"[number of items] * (READ + 2 * WRITE), TX",
			false,
			true,

			"mortoncurve3d"
	),

	new Cmd(
			"MC3POINT",

			"MC3POINT key x y z number [start]",

			"Gets all subkeys stored in 'point' with 3D coordinates <i>x</i>, <i>y</i> and <i>z</i>.<br />" .
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

			"mortoncurve3d"
	),

	new Cmd(
			"MC3RANGE",

			"MC3RANGE key x_min x_max y_min y_max z_min z_max number [start]",

			"Gets all subkeys stored in 'rectangle' with 3D coordinates <i>x_min</i> and <i>y_min</i> to <i>x_max</i> and <i>y_max</i> to <i>z_max</i> and <i>z_max</i>.<br />" .
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

			"mortoncurve3d"
	),

	new Cmd(
			"MC3RANGENAIVE",

			"MC3RANGENAIVE key x_min x_max y_min y_max z_min z_max number [start]",

			"Naive, non performant version of MC3XNGET. Made for small datasets and for testing.<br />" .
			"See MC3RANGE for more information.<br />" .
			"Uses <b>uint32_t</b> for coordinates.<br />" .
			"Technically the data lookup complexity is single READ, but on large datasets, it might need to read complete dataset.",
			"array",
			"results",
			"1.3.10.2",
			"READ",
			false,
			false,

			"mortoncurve3d"
	),

	new Cmd(
			"MC3ENCODE",

			"MC3ENCODE x y z",

			"Encode <i>x</i>, <i>y</i> and <i>z</i> to morton code hex string.<br />" .
			"Uses <b>uint32_t</b> for coordinates.",
			"string",
			"morton code hex string",
			"1.3.10.2",
			"READ",
			false,
			null,

			"mortoncurve3d"
	),

	new Cmd(
			"MC3DECODE",

			"MC3DECODE hex",

			"Decode <i>morton code hex string</i> to x, y, z.",
			"array",
			"x, y and z",
			"1.3.10.2",
			"READ",
			false,
			null,

			"mortoncurve3d"
	),

);

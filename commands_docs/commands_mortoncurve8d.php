<?php
return array(
	new Cmd(
			"MC8GET",

			"MC8GET key subkey",

			"Get value of the <i>subkey</i> stored in <i>key</i>.",
			"string",
			"value of the subkey",
			"1.3.12",
			"2 * READ",
			false,
			false,

			"mortoncurve8d"
	),

	new Cmd(
			"MC8MGET",

			"MC8MGET key subkey [subkey]...",

			"Get values of the <i>subkey</i> stored in <i>key</i>.",
			"array",
			"values of the items",
			"1.3.12",
			"[number of items] * 2 * READ",
			false,
			false,

			"mortoncurve8d"
	),

	new Cmd(
			"MC8EXISTS",

			"MC8EXISTS key subkey",

			"Check if <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"0 if the item do not exists.<br />" .
			"1 if the item exists.",
			"1.3.12",
			"2 * READ",
			false,
			false,

			"mortoncurve8d"
	),

	new Cmd(
			"MC8SCORE",

			"MC8SCORE key subkey",

			"Get score of the <i>subkey</i> stored in <i>key</i>.",
			"array",
			"x, y and z",
			"1.3.12",
			"READ",
			false,
			false,

			"mortoncurve8d"
	),

	new Cmd(
			"MC8ADD",

			"MC8ADD key subKey v0 ... v7 value [subKey v0 ... v7 value]...",

			"Set <i>subkey</i> with 8D coordinates <i>v0</i> ...<i>v7</i> and <i>value</i> in <i>key</i>.<br />" .
			"Uses <b>uint8_t</b> for coordinates.",
			"OK",
			"OK",
			"1.3.12",
			"[number of items] * (READ + 3 * WRITE), TX",
			false,
			true,

			"mortoncurve8d"
	),

	new Cmd(
			"MC8REM",

			"MC8REM key subkey [subkey]...",

			"Removes <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"Always return 1",
			"1.3.12",
			"[number of items] * (READ + 2 * WRITE), TX",
			false,
			true,

			"mortoncurve8d"
	),

	new Cmd(
			"MC8POINT",

			"MC8POINT key v0 ... v7 number [start]",

			"Gets all subkeys stored in 'point' with 8D coordinates <i>v0</i> ... <i>v7</i>.<br />" .
			"Uses <b>uint8_t</b> for coordinates.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"Return up to <i>number</i> of pairs.<br />" .
			"Returns up to ~32'000 elements.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.12",
			"READ",
			false,
			false,

			"mortoncurve8d"
	),

	new Cmd(
			"MC8RANGE",

			"MC8RANGE key v0_min v0_max ... v7_min v7_max number [start]",

			"Gets all subkeys stored in 'rectangle' with 8D coordinates <i>v0_min</i> ... <i>v0_min</i> to <i>v7_min</i> ... <i>v7_min</i>.<br />" .
			"Uses <b>uint8_t</b> for coordinates.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"Return up to <i>number</i> of pairs.<br />" .
			"Returns up to ~32'000 elements.<br />" .
			"Note this command is useful only on very large datasets.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.12",
			"N * READ, were N &isin; { 1 .. 9 }",
			false,
			false,

			"mortoncurve8d"
	),

	new Cmd(
			"MC8RANGENAIVE",

			"MC8RANGENAIVE key v0_min v0_max ... v7_min v7_max number [start]",

			"Naive, non performant version of MC8XNGET. Made for small datasets and for testing.<br />" .
			"See MC8RANGE for more information.<br />" .
			"Uses <b>uint8_t</b> for coordinates.<br />" .
			"Technically the data lookup complexity is single READ, but on large datasets, it might need to read complete dataset.",
			"array",
			"results",
			"1.3.12",
			"READ",
			false,
			false,

			"mortoncurve8d"
	),

	new Cmd(
			"MC8ENCODE",

			"MC8ENCODE v0 ... v7",

			"Encode <i>v0</i> ... <i>v7</i> to morton code hex string.<br />" .
			"Uses <b>uint8_t</b> for coordinates.",
			"string",
			"morton code hex string",
			"1.3.12",
			"READ",
			false,
			null,

			"mortoncurve8d"
	),

	new Cmd(
			"MC8DECODE",

			"MC8DECODE hex",

			"Decode <i>morton code hex string</i> to v0 ... v7.",
			"array",
			"v0 ... v7",
			"1.3.12",
			"READ",
			false,
			null,

			"mortoncurve8d"
	),

);

<?php
return array(
	new Cmd(
			"MC16GET",

			"MC16GET key subkey",

			"Get value of the <i>subkey</i> stored in <i>key</i>.",
			"string",
			"value of the subkey",
			"1.3.12",
			"2 * READ",
			false,
			false,

			"mortoncurve16d"
	),

	new Cmd(
			"MC16MGET",

			"MC16MGET key subkey [subkey]...",

			"Get values of the <i>subkey</i> stored in <i>key</i>.",
			"array",
			"values of the items",
			"1.3.12",
			"[number of items] * 2 * READ",
			false,
			false,

			"mortoncurve16d"
	),

	new Cmd(
			"MC16EXISTS",

			"MC16EXISTS key subkey",

			"Check if <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"0 if the item do not exists.<br />" .
			"1 if the item exists.",
			"1.3.12",
			"2 * READ",
			false,
			false,

			"mortoncurve16d"
	),

	new Cmd(
			"MC16SCORE",

			"MC16SCORE key subkey",

			"Get score of the <i>subkey</i> stored in <i>key</i>.",
			"array",
			"x, y and z",
			"1.3.12",
			"READ",
			false,
			false,

			"mortoncurve16d"
	),

	new Cmd(
			"MC16ADD",

			"MC16ADD key subKey v0 ... v15 value [subKey v0 ... v15 value]...",

			"Set <i>subkey</i> with 16D coordinates <i>v0</i> ...<i>v15</i> and <i>value</i> in <i>key</i>.<br />" .
			"Uses <b>uint8_t</b> for coordinates.",
			"OK",
			"OK",
			"1.3.12",
			"[number of items] * (READ + 3 * WRITE), TX",
			false,
			true,

			"mortoncurve16d"
	),

	new Cmd(
			"MC16REM",

			"MC16REM key subkey [subkey]...",

			"Removes <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"Always return 1",
			"1.3.12",
			"[number of items] * (READ + 2 * WRITE), TX",
			false,
			true,

			"mortoncurve16d"
	),

	new Cmd(
			"MC16POINT",

			"MC16POINT key v0 ... v15 number [start]",

			"Gets all subkeys stored in 'point' with 16D coordinates <i>v0</i> ... <i>v15</i>.<br />" .
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

			"mortoncurve16d"
	),

	new Cmd(
			"MC16RANGE",

			"MC16RANGE key v0_min v0_max ... v15_min v15_max number [start]",

			"Gets all subkeys stored in 'rectangle' with 16D coordinates <i>v0_min</i> ... <i>v0_min</i> to <i>v15_min</i> ... <i>v15_min</i>.<br />" .
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

			"mortoncurve16d"
	),

	new Cmd(
			"MC16RANGENAIVE",

			"MC16RANGENAIVE key v0_min v0_max ... v15_min v15_max number [start]",

			"Naive, non performant version of MC16XNGET. Made for small datasets and for testing.<br />" .
			"See MC16RANGE for more information.<br />" .
			"Uses <b>uint8_t</b> for coordinates.<br />" .
			"Technically the data lookup complexity is single READ, but on large datasets, it might need to read complete dataset.",
			"array",
			"results",
			"1.3.12",
			"READ",
			false,
			false,

			"mortoncurve16d"
	),

	new Cmd(
			"MC16ENCODE",

			"MC16ENCODE v0 ... v15",

			"Encode <i>v0</i> ... <i>v15</i> to morton code hex string.<br />" .
			"Uses <b>uint8_t</b> for coordinates.",
			"string",
			"morton code hex string",
			"1.3.12",
			"READ",
			false,
			null,

			"mortoncurve16d"
	),

	new Cmd(
			"MC16DECODE",

			"MC16DECODE hex",

			"Decode <i>morton code hex string</i> to v0 ... v15.",
			"array",
			"v0 ... v15",
			"1.3.12",
			"READ",
			false,
			null,

			"mortoncurve16d"
	),

);

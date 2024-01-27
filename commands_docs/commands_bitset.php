<?php
return array(
	new Cmd(
			"BITSET",

			"BITSET / BITMSET / SETBIT key index value [index value]...",

			"Set bit(s) of the <i>key</i> with <i>index</i> to <i>value</i> (0/1).<br />" .
			"Read Bitset information document.",
			"OK",
			"OK",
			"1.2.17",
			"READ + WRITE",
			true,
			true,

			"bitset"
	),

	new Cmd(
			"BITGET",

			"BITGET / GETBIT key index",

			"Get bit of the <i>key</i> with <i>index</i>.<br />" .
			"Read Bitset information document.",
			"string (int)",
			"0 / 1",
			"1.2.17",
			"READ",
			true,
			false,

			"bitset"
	),

	new Cmd(
			"BITMGET",

			"BITMGET key index [index]...",

			"Get bit(s) of the <i>key</i> with <i>index</i><br />" .
			"Read Bitset information document.",
			"array",
			"0 / 1",
			"1.3.0",
			"READ",
			true,
			false,

			"bitset"
	),

	new Cmd(
			"BITCOUNT",

			"BITCOUNT key",

			"Calculate bitcount (popcount) of the <i>key</i>,<br />" .
			"e.g. number of bits set to 1.",
			"string (int)",
			"bitcount",
			"1.2.17",
			"READ",
			true,
			false,

			"bitset"
	),

	new Cmd(
			"BITMAX",

			"BITMAX",

			"Return maximum suported index,<br />" .
			"it is limited of max value size - currently 1MB * 8 bits = 8'388'608 bits.",
			"string (int)",
			"Maximum suported index.",
			"1.2.17",
			"n/a",
			false,
			null,

			"bitset"
	),
);

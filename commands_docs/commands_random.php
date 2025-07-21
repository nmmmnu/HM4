<?php
return array(
	new Cmd(
			"RANDOM",

			"RANDOM / RAND [seed=0]",

			"Returns pregenerated stable random number from global array",
			"int",
			"Returns pregenerated stable random number from global array.",
			"1.3.12.1",
			"n/a",
			null,
			null,

			"random"
	),
	new Cmd(
			"MRANDOM",

			"MRANDOM / MRAND seed0 seed1 ...",

			"Returns several pregenerated stable random number from global array",
			"array",
			"Returns several pregenerated stable random number from global array.",
			"1.3.12.1",
			"n/a",
			null,
			null,

			"random"
	),
);


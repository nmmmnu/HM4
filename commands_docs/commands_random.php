<?php
return array(
	new Cmd(
			"RANDOM64",

			"RANDOM64 / RAND64 [seed=0]",

			"Returns pregenerated stable uint64_t random number from global array",
			"int",
			"Returns pregenerated stable uint64_trandom number from global array.",
			"1.3.12.1",
			"n/a",
			null,
			null,

			"random"
	),
	new Cmd(
			"RANDOM32",

			"RANDOM32 / RAND32 [seed=0]",

			"Returns pregenerated stable uint32_t random number from global array",
			"int",
			"Returns pregenerated stable uint32_t random number from global array.",
			"1.3.12.1",
			"n/a",
			null,
			null,

			"random"
	),
	new Cmd(
			"RANDOM16",

			"RANDOM16 / RAND16 [seed=0]",

			"Returns pregenerated stable uint16_t random number from global array",
			"int",
			"Returns pregenerated stable uint16_t random number from global array.",
			"1.3.12.1",
			"n/a",
			null,
			null,

			"random"
	),
	new Cmd(
			"RANDOM8",

			"RANDOM8 / RAND8 [seed=0]",

			"Returns pregenerated stable uint8_t random number from global array",
			"int",
			"Returns pregenerated stable uint8_t random number from global array.",
			"1.3.12.1",
			"n/a",
			null,
			null,

			"random"
	),
	new Cmd(
			"MRANDOM64",

			"MRANDOM64 / MRAND64 seed0 seed1 ...",

			"Returns several pregenerated uint64_t stable random number from global array",
			"array",
			"Returns several pregenerated uint64_t stable random number from global array.",
			"1.3.12.1",
			"n/a",
			null,
			null,

			"random"
	),
	new Cmd(
			"MRANDOM32",

			"MRANDOM32 / MRAND32 seed0 seed1 ...",

			"Returns several pregenerated uint32_t stable random number from global array",
			"array",
			"Returns several pregenerated uint32_t stable random number from global array.",
			"1.3.12.1",
			"n/a",
			null,
			null,

			"random"
	),
	new Cmd(
			"MRANDOM16",

			"MRANDOM16 / MRAND16 seed0 seed1 ...",

			"Returns several pregenerated uint16_t stable random number from global array",
			"array",
			"Returns several pregenerated uint16_t stable random number from global array.",
			"1.3.12.1",
			"n/a",
			null,
			null,

			"random"
	),
	new Cmd(
			"MRANDOM8",

			"MRANDOM8 / MRAND8 seed0 seed1 ...",

			"Returns several pregenerated uint8_t stable random number from global array",
			"array",
			"Returns several pregenerated uint8_t stable random number from global array.",
			"1.3.12.1",
			"n/a",
			null,
			null,

			"random"
	),
);


<?php
return array(
	new Cmd(
			"RANDOM*",

			"RANDOM8 / RANDOM16 / RANDOM32 / RANDOM64  [seed=0]<br />" .
			"RAND8 / RAND16 / RAND32 / RAND64 [seed=0]",

			"Returns pregenerated stable uint8_t / uint16_t / uint32_t / uint64_t random number from global array",
			"int",
			"Returns pregenerated stable uint8_t / uint16_t / uint32_t / uint64_t random number from global array.",
			"1.3.12.1",
			"n/a",
			null,
			null,

			"random"
	),
	new Cmd(
			"MRANDOM*",

			"MRANDOM8 / MRANDOM16 / MRANDOM32 / MRANDOM64 seed0 seed1 ...<br />" .
			"MRAND8 / MRAND16 / MRAND32 / MRAND64 seed0 seed1 ...",

			"Returns several pregenerated uint8_t / uint16_t / uint32_t / uint64_t stable random number from global array",
			"array",
			"Returns several pregenerated uint8_t / uint16_t / uint32_t / uint64_t stable random number from global array.",
			"1.3.12.1",
			"n/a",
			null,
			null,

			"random"
	),
);


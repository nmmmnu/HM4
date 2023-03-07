<?php
return array(
	new Cmd(
			"MURMUR",

			"MURMUR / MURMURHASH64A string [seed=0] [modulo=0]",

			"Returns murmur_hash64a from value",
			"string",
			"Returns murmur_hash64a from value with specific seed and specific modulo.<br />" .
			"e.g. murmur_hash64a(value, seed) % modulo<br />" .
			"Very useful for debugging.",
			"1.3.0",
			"n/a",
			null,
			null,

			"murmur"
	),
);

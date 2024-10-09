<?php
return array(
	new Cmd(
			"TDADD",

			"TDADD key td_capacity delta value [value]...",

			"Add <i>value</i>'s into the TDigests with capacity <i>td_capacity</i>.<br />" .
			"<i>td_capacity</i> can be from 16 to over 100'000.<br />" .
			"Read TDigest information document.",
			"bool",
			"always returns 1",
			"1.3.8",
			"READ + WRITE",
			false,
			true,

			"tdigest"
	),

	new Cmd(
			"TDADDWEIGHT",

			"TDADDWEIGHT key td_capacity delta value weight [value weight]...",

			"Add <i>value</i>'s with <i>weight</i> into the TDigests with capacity <i>td_capacity</i>.<br />" .
			"<i>td_capacity</i> can be from 16 to over 100'000.<br />" .
			"<i>weight</i> is integer larger or equal to 1.<br />" .
			"Read TDigest information document.",
			"bool",
			"always return 1",
			"1.3.7.8",
			"READ + WRITE",
			false,
			true,

			"tdigest"
	),

	new Cmd(
			"TDRESERVE",

			"TDRESERVE key td_capacity",

			"Create new, empty TDigests with capacity <i>td_capacity</i>.<br />" .
			"<i>td_capacity</i> can be from 16 to over 100'000.<br />" .
			"Read TDigest information document.",
			"bool",
			"always returns 1",
			"1.3.7.8",
			"READ + WRITE",
			false,
			true,

			"tdigest"
	),

	new Cmd(
			"TDMERGE",

			"TDMERGE key td_capacity delta src_key [src_key]...",

			"Merge <i>src_key</i> with capacity <i>td_capacity</i> into <i>key</i> with capacity <i>td_capacity</i>.<br />" .
			"<i>td_capacity</i> can be from 16 to over 100'000.<br />" .
			"Read TDigest information document.",
			"bool",
			"always return 1",
			"1.3.7.8",
			"n * READ + WRITE",
			false,
			true,

			"tdigest"
	),

	new Cmd(
			"TDMERGECAPACITY",

			"TDMERGECAPACITY / TDMERGECAP key td_capacity delta src_key src_td_capacity [src_key src_td_capacity]...",

			"Merge <i>src_key</i> with capacity <i>src_td_capacity</i> into <i>key</i> with capacity <i>td_capacity</i>.<br />" .
			"<i>td_capacity</i> and <i>src_td_capacity</i> can be from 16 to over 100'000.<br />" .
			"Read TDigest information document.",
			"bool",
			"always return 1",
			"1.3.7.8",
			"n * READ + WRITE",
			false,
			true,

			"tdigest"
	),

	new Cmd(
			"TDPERCENTILE",

			"TDPERCENTILE / TDQUANTILE key td_capacity percentile",

			"Estimate percentile (quantile) from <i>key</i> with capacity <i>td_capacity</i>.<br />" .
			"<i>td_capacity</i> can be from 16 to over 100'000.<br />" .
			"<i>percentile</i> is floating point from 0.0 to 1.0 .<br />" .
			"Read TDigest information document.",
			"string",
			"floating point returned as string",
			"1.3.7.8",
			"n * READ + WRITE",
			false,
			true,

			"tdigest"
	),

	new Cmd(
			"TDMPERCENTILE",

			"TDMPERCENTILE / TDMQUANTILE key td_capacity percentile [percentile]...",

			"Estimate percentile (quantile) from <i>key</i> with capacity <i>td_capacity</i>.<br />" .
			"<i>td_capacity</i> can be from 16 to over 100'000.<br />" .
			"<i>percentile</i> is floating point from 0.0 to 1.0 .<br />" .
			"Read TDigest information document.",
			"array",
			"array of floating point values as string",
			"1.3.7.8",
			"n * READ + WRITE",
			false,
			true,

			"tdigest"
	),


);

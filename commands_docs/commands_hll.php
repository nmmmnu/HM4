<?php
return array(
	new Cmd(
			"PFADD",

			"PFADD / HLLADD key value [value]...",

			"Add <i>value</i> into HLL <i>key</i>.<br />" .
			"Read HLL information document.",
			"string (int)",
			"0 = count is unchanged, 1 = count is increased",
			"1.2.17",
			"READ + [number of keys] * WRITE",
			true,
			true,

			"hll"
	),

	new Cmd(
			"PFRESERVE",

			"PFRESERVE / HLLRESERVE key",

			"Create new, empty HLL <i>key</i>.<br />" .
			"Read HLL information document.",
			"OK",
			"OK",
			"1.2.30",
			"READ + WRITE",
			false,
			true,

			"hll"
	),

	new Cmd(
			"PFCOUNT",

			"PFCOUNT / HLLCOUNT [key]...",

			"Estimate count of HLL union of provided <i>key</i>.<br />" .
			"Works with single key too, but returns standard estimation.<br />" .
			"Works without key too, but returns 0.<br />" .
			"Read HLL information document.",
			"string (int)",
			"estimated count",
			"1.2.17",
			"READ",
			true,
			false,

			"hll"
	),

	new Cmd(
			"PFADDCOUNT",

			"PFADDCOUNT / HLLADDCOUNT key value [value]...",

			"Add <i>value</i> into HLL <i>key</i>, " .
			"then wstimate count of HLL union of provided <i>key</i>.<br />" .
			"Read HLL information document.",
			"string (int)",
			"estimated count",
			"1.3.7.7",
			"READ + WRITE",
			true,
			false,

			"hll"
	),

	new Cmd(
			"PFINTERSECT",

			"PFINTERSECT / HLLINTERSECT [key1] [key2] [key3] [key4] [key5]",

			"Estimate count of the intersection of up to 5 keys, using HLL unions.<br />" .
			"Works with single key too, but returns standard estimation.<br />" .
			"Works without key too, but returns 0.<br />" .
			"Read HLL information document.",
			"string (int)",
			"estimated count of intersection",
			"1.2.17",
			"[number of keys] * READ",
			false,
			false,

			"hll"
	),

	new Cmd(
			"PFMERGE",

			"PFMERGE / HLLMERGE dest_key key [key]... ",

			"Make a HLL union of <i>key</i>... and store it in <i>dest_key</i><br />" .
			"If <i>dest_key</i> contains valid HLL value, its value used in the merge process too.<br />" .
			"Works with single key too.",
			"OK",
			"Value of the removed element or empty string.",
			"1.2.17",
			"[number of keys] * READ + WRITE",
			true,
			false,

			"hll"
	),

	new Cmd(
			"PFBITS",

			"PFBITS / HLLBITS",

			"return HLL bits",
			"string (int)",
			"HLL bits",
			"1.2.17",
			"n/a",
			false,
			null,

			"hll"
	),

	new Cmd(
			"PFERROR",

			"PFERROR / HLLERROR",

			"return HLL error rate",
			"string (int)",
			"HLL error rate as percent, but multiplied to 1'00'00",
			"1.2.17",
			"n/a",
			false,
			null,

			"hll"
	),
);

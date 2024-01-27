<?php
return array(
	new Cmd(
			"BFADD",

			"BFADD / BFMADD key bits hash value [value]...",

			"Add (insert) <i>value</i>'s into the bloom filter with size <i>bits</i> and number of <i>hash</i> functions.<br />" .
			"Read BF information document.",
			"OK",
			"OK",
			"1.3.1",
			"READ + WRITE",
			false,
			true,

			"bf"
	),

	new Cmd(
			"BFRESERVE",

			"BFRESERVE key bits hash",

			"Create new, empty bloom filter with size <i>bits</i> number of <i>hash</i> functions.<br />" .
			"Value of <i>hash</i> is not used.<br />" .
			"Read BF information document.",
			"OK",
			"OK",
			"1.3.1",
			"READ + WRITE",
			false,
			true,

			"bf"
	),

	new Cmd(
			"BFEXISTS",

			"BFEXISTS key bits hash value",

			"Check if <i>value</i> is present in the bloom filter with size <i>bits</i> number of <i>hash</i> functions.<br />" .
			"Read BF information document.",
			"bool",
			"0 if value is not present<br />" .
			"1 if value is MAY BE present",
			"1.3.1",
			"READ",
			false,
			false,

			"bf"
	),

	new Cmd(
			"BFMEXISTS",

			"BFMEXISTS key bits hash value [value]...",

			"Check if <i>value</i>'s are present in the bloom filter with size <i>bits</i> number of <i>hash</i> functions.<br />" .
			"Read Bitset information document.",
			"array",
			"array of bools" .
			"0 if value is not present<br />" .
			"1 if value is MAY BE present",
			"1.3.1",
			"READ",
			false,
			false,

			"bf"
	),
);

<?php
return array(
	new Cmd(
			"CBFADD",

			"CBFADD / CBFINCR / CBFINCRBY key width hash integer_size value inc [value inc]...",

			"Add (insert) <i>value</i> into the counting bloom filter with size <i>width</i> and number of <i>hash</i> and <i>integer_size</i> integers.<br />" .
			"<i>inc</i> specify how many times you want to add it / how much to increase it.<br />" .
			"Read CBF information document.",
			"OK",
			"OK",
			"1.3.1",
			"READ + WRITE",
			false,
			true,

			"cms"
	),

	new Cmd(
			"CBFREM",

			"CBFREM / CBFDECR / CBFDECRBY key width hash integer_size value inc [value inc]...",

			"Remove (insert) <i>value</i> from the counting bloom filter with size <i>width</i> and number of <i>hash</i> and <i>integer_size</i> integers.<br />" .
			"<i>inc</i> specify how many times you want to remove it / how much to decrease it.<br />" .
			"Read CBF information document.",
			"OK",
			"OK",
			"1.3.7.7",
			"READ + WRITE",
			false,
			true,

			"cms"
	),

	new Cmd(
			"CBFRESERVE",

			"CBFRESERVE key key width hash integer_size",

			"Create new, empty counting bloom filter with size <i>width</i> and number of <i>hash</i> and <i>integer_size</i> integers.<br />" .
			"Read CBF information document.",
			"OK",
			"OK",
			"1.3.1",
			"READ + WRITE",
			false,
			true,

			"cms"
	),

	new Cmd(
			"CBFCOUNT",

			"CBFCOUNT key key width hash integer_size value",

			"Check estimated count of <i>value</i> that may be present into the counting bloom filter with size <i>width</i> and number of <i>hash</i> and <i>integer_size</i> integers.<br />" .
			"Read CBF information document.",
			"int",
			"estimated count",
			"1.3.1",
			"READ",
			false,
			false,

			"cms"
	),

	new Cmd(
			"CBFMCOUNT",

			"CBFMCOUNT key width hash integer_size value [value]...",

			"Check estimated count of <i>value</i> that may be present into the counting bloom filter with size <i>width</i> and number of <i>hash</i> and <i>integer_size</i> integers.<br />" .
			"Read CBF information document.",
			"array",
			"array of estimated counts",
			"1.3.1",
			"READ",
			false,
			false,

			"cms"
	),

	new Cmd(
			"CBFADDCOUNT",

			"CBFADDCOUNT key width hash integer_size value inc",

			"Add (insert) <i>value</i> into the counting bloom filter with size <i>width</i> and number of <i>hash</i> and <i>integer_size</i> integers.<br />" .
			"<i>inc</i> specify how many times you want to add it / how much to increase it.<br />" .
			"Read CBF information document.",
			"int",
			"estimated count",
			"1.3.1",
			"READ + WRITE",
			false,
			true,

			"cms"
	),

	new Cmd(
			"CBFREMCOUNT",

			"CBFREMCOUNT key width hash integer_size value inc",

			"Remove (insert) <i>value</i> from the counting bloom filter with size <i>width</i> and number of <i>hash</i> and <i>integer_size</i> integers.<br />" .
			"<i>inc</i> specify how many times you want to remove it / how much to decrease it.<br />" .
			"Read CBF information document.",
			"int",
			"estimated count",
			"1.3.1",
			"READ + WRITE",
			false,
			true,

			"cms"
	),

	new Cmd(
			"CBFADDCOUNT",

			"CBFADDCOUNT key width hash integer_size value inc",

			"Add (insert) <i>value</i> into the counting bloom filter with size <i>width</i> and number of <i>hash</i> and <i>integer_size</i> integers.<br />" .
			"<i>inc</i> specify how many times you want to add it / how much to increase it.<br />" .
			"Read CBF information document.",
			"int",
			"estimated count",
			"1.3.7.7",
			"READ + WRITE",
			false,
			true,

			"cms"
	),
);

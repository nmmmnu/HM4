<?php
return array(
	new Cmd(
			"HHINCR",

			"HHINCR key hh_count hh_value_size value count [value count]...",

			"Add (insert) <i>value</i>'s into the heavy hitter with size <i>hh_count</i> and fixed value size of <i>hh_value_size</i>.<br />" .
			"Bigger values are 'better'<br />" .
			"<i>hh_count</i> can be from 1 to 200.<br />" .
			"Supported sizes are:<br />".
			"-  32 (gives you string size of  31)<br />".
			"-  40 (gives you string size of  39) - IP6 compatible<br />".
			"-  64 (gives you string size of  63)<br />".
			"- 128 (gives you string size of 127)<br />".
			"- 256 (gives you string size of 255) - Pascal string compatible :)<br />".
			"Read HH information document.",
			"bool",
			"0 if all the items is not qualified for insert<br />" .
			"1 if at least one item is qualified for insert",
			"1.3.7.7",
			"READ + WRITE",
			false,
			true,

			"hh"
	),

	new Cmd(
			"HHDECR",

			"HHDECR key hh_count hh_value_size value count [value count]...",

			"Add (insert) <i>value</i>'s into the heavy hitter with size <i>hh_count</i> and fixed value size of <i>hh_value_size</i>.<br />" .
			"Smaller values are 'better'<br />" .
			"<i>hh_count</i> can be from 1 to 200.<br />" .
			"Check <i>HHINCR</i> for supported sizes.<br />".
			"Read HH information document.",
			"bool",
			"0 if all the items is not qualified for insert<br />" .
			"1 if at least one item is qualified for insert",
			"1.3.7.7",
			"READ + WRITE",
			false,
			true,

			"hh"
	),

	new Cmd(
			"HHRESERVE",

			"HHRESERVE key hh_count hh_value_size",

			"Create new, empty heavy hitter with size <i>hh_count</i> and fixed value size of <i>hh_value_size</i>.<br />" .
			"<i>hh_count</i> can be from 1 to 200.<br />" .
			"Check <i>HHINCR</i> for supported sizes.<br />".
			"Read HH information document.",
			"int",
			"always return 1",
			"1.3.1",
			"READ + WRITE",
			false,
			true,

			"hh"
	),

	new Cmd(
			"HHGET",

			"HHGET key hh_count hh_value_size",

			"Get heavy hitter with size <i>hh_count</i> and fixed value size of <i>hh_value_size</i>.<br />" .
			"<i>hh_count</i> can be from 1 to 200.<br />" .
			"Check <i>HHINCR</i> for supported sizes.<br />".
			"Note values are returned nonsorted. You will need to sort values yourself.<br />".
			"Read HH information document.",
			"array",
			"key => count in a way similar to <i>HGETALL</i>",
			"1.3.1",
			"READ",
			false,
			false,

			"hh"
	),

);

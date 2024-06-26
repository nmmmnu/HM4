<?php
return array(
	new Cmd(
			"MGADD",

			"MGADD / MGINCR key mg_count mg_value_size value [value]...",

			"Add <i>value</i>'s into the Misra Gries heavy hitter with size <i>mg_count</i> and fixed value size of <i>mg_value_size</i>.<br />" .
			"<i>mg_count</i> can be from 1 to 200.<br />" .
			"Supported sizes are:<br />".
			"-  32 (gives you string size of  31)<br />".
			"-  40 (gives you string size of  39) - IP6 compatible<br />".
			"-  64 (gives you string size of  63)<br />".
			"- 128 (gives you string size of 127)<br />".
			"- 256 (gives you string size of 255) - Pascal string compatible :)<br />".
			"Read Misra Gries information document.",
			"int",
			"0 if all the items is not qualified for insert<br />" .
			"1 if at least one item is qualified for insert",
			"1.3.7.8",
			"READ + WRITE",
			false,
			true,

			"mg"
	),

	new Cmd(
			"MGADDGET",

			"MGADDGET key mg_count mg_value_size value",

			"Add <i>value</i>'s into the Misra Gries heavy hitter with size <i>mg_count</i> and fixed value size of <i>mg_value_size</i>.<br />" .
			"<i>mg_count</i> can be from 1 to 200.<br />" .
			"Check <i>MGADD</i> for supported sizes.<br />".
			"Read Misra Gries information document.",
			"int",
			"0 if the item is not qualified for insert<br />" .
			"score of the item, if the item is qualified for insert",
			"1.3.7.8",
			"READ + WRITE",
			false,
			true,

			"mg"
	),

	new Cmd(
			"MGRESERVE",

			"MGRESERVE key mg_count mg_value_size",

			"Create new, empty Misra Gries heavy hitter with size <i>mg_count</i> and fixed value size of <i>mg_value_size</i>.<br />" .
			"<i>mg_count</i> can be from 1 to 200.<br />" .
			"Check <i>MGADD</i> for supported sizes.<br />".
			"Read Misra Gries information document.",
			"int",
			"always return 1",
			"1.3.7.8",
			"READ + WRITE",
			false,
			true,

			"mg"
	),

	new Cmd(
			"MGGET",

			"MGGET key mg_count mg_value_size",

			"Get Misra Gries heavy hitter with size <i>mg_count</i> and fixed value size of <i>mg_value_size</i>.<br />" .
			"<i>mg_count</i> can be from 1 to 200.<br />" .
			"Check <i>MGADD</i> for supported sizes.<br />".
			"Note values are returned nonsorted. You will need to sort values yourself.<br />".
			"Read Misra Gries information document.",
			"array",
			"key => count in a way similar to <i>HGETALL</i>",
			"1.3.7.8",
			"READ",
			false,
			false,

			"mg"
	),

);

<?php
return array(
	new Cmd(
			"RSADD",

			"RSADD key rs_count rs_value_size value [value]...",

			"Add <i>value</i>'s into the Reservoir Sampling with size <i>rs_count</i> and fixed value size of <i>rs_value_size</i>.<br />" .
			"<i>rs_count</i> can be from 1 to 4096.<br />" .
			"Supported sizes are:<br />".
			"-  32 (gives you string size of  31)<br />".
			"-  40 (gives you string size of  39) - IP6 compatible<br />".
			"-  64 (gives you string size of  63)<br />".
			"- 128 (gives you string size of 127)<br />".
			"- 256 (gives you string size of 255) - Pascal string compatible :)<br />".
			"Read Reservoir Sampling information document.",
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
			"RSRESERVE",

			"RSRESERVE key rs_count rs_value_size",

			"Create new, empty Reservoir Sampling with size <i>rs_count</i> and fixed value size of <i>rs_value_size</i>.<br />" .
			"<i>rs_count</i> can be from 1 to 4096.<br />" .
			"Check <i>RSADD</i> for supported sizes.<br />".
			"Read Reservoir Sampling information document.",
			"int",
			"always return 1",
			"1.3.7.8",
			"READ + WRITE",
			false,
			true,

			"mg"
	),

	new Cmd(
			"RSGET",

			"RSGET key rs_count rs_value_size",

			"Get samples from Reservoir Sampling with size <i>rs_count</i> and fixed value size of <i>rs_value_size</i>.<br />" .
			"<i>rs_count</i> can be from 1 to 4096.<br />" .
			"Check <i>RSADD</i> for supported sizes.<br />".
			"Read Reservoir Sampling information document.",
			"array",
			"array of samles",
			"1.3.7.8",
			"READ",
			false,
			false,

			"mg"
	),


	new Cmd(
			"RSGETCOUNT",

			"RSGETCOUNT key rs_count rs_value_size",

			"Get count of insertions made in Reservoir Sampling with size <i>rs_count</i> and fixed value size of <i>rs_value_size</i>.<br />" .
			"<i>rs_count</i> can be from 1 to 4096.<br />" .
			"Check <i>RSADD</i> for supported sizes.<br />".
			"Read Reservoir Sampling information document.",
			"int",
			"count of insertions made",
			"1.3.7.8",
			"READ",
			false,
			false,

			"mg"
	),
);

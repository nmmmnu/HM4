<?php
return array(
	new Cmd(
			"CVPUSH",

			"CVPUSH key type value [index] [value]",

			"CVPUSH <i>value</i> at the end of the array <i>key</i> of <i>type</i>.<br />" .
			"Read CompactArray information document.",
			"OK",
			"OK",
			"1.3.2",
			"READ + WRITE",
			false,
			true,

			"cv"
	),

	new Cmd(
			"CVPOP",

			"CVPOP key type",

			"Remove last element from the array <i>key</i> of <i>type</i>.<br />" .
			"Read CompactArray information document.",
			"string (int)",
			"value of the element",
			"1.3.2",
			"READ + WRITE",
			false,
			true,

			"cv"
	),

	new Cmd(
			"CVGET",

			"CVGET key type index",

			"Get element with <i>index</i> from the array <i>key</i> of <i>type</i>.<br />" .
			"Read CompactArray information document.",
			"string (int)",
			"value of the element",
			"1.3.2",
			"READ",
			false,
			false,

			"cv"
	),

	new Cmd(
			"CVMGET",

			"CVMGET key type index [index1]",

			"Get elements with <i>index</i>, <i>index1</i>... from the array <i>key</i> of <i>type</i>.<br />" .
			"Read Bitset information document.",
			"array",
			"values of the elements",
			"1.3.2",
			"READ",
			false,
			false,

			"cv"
	),

	new Cmd(
			"CVGETRANGE",

			"CVMGET key type index-start index-end",

			"Get elements with indexes from <i>index-start</i> to <i>index-end</i> from the array <i>key</i> of <i>type</i>.<br />" .
			"Read Bitset information document.",
			"array",
			"values of the elements",
			"1.3.2",
			"READ",
			false,
			false,

			"cv"
	),

	new Cmd(
			"CVLEN",

			"CVLEN key",

			"Get number of elements from the array <i>key</i> of <i>type</i>.<br />" .
			"Read CompactArray information document.",
			"string (int)",
			"size of the array",
			"1.3.2",
			"READ",
			false,
			false,

			"cv"
	),

	new Cmd(
			"CVMAX",

			"CVMAX type",

			"Return maximum suported size from arrays of <i>type</i>,<br />" .
			"it is limited of max value size.<br />" .
			"Read CompactArray information document.",
			"string (int)",
			"maximum suported size of the array",
			"1.3.2",
			"n/a",
			false,
			null,

			"cv"
	),
);

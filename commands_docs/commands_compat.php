<?php
return array(
	new Cmd(
			"SELECT",

			"SELECT database",

			"Provided for compatibility",
			"OK",
			"OK",
			"1.2.16",
			"n/a",
			true,
			null,

			"compat"
	),

	new Cmd(
			"RESET",

			"RESET",

			"Provided for compatibility",
			"OK",
			"OK",
			"1.2.18",
			"n/a",
			true,
			null,

			"compat"
	),

	new Cmd(
			"TYPE",

			"TYPE key",

			"Returns type of given <i>key</i>.<br />" .
			"For compatibility, always return 'string'",
			"string",
			"Always return 'string'",
			"1.2.16",
			"n/a",
			true,
			null,

			"compat"
	),

	new Cmd(
			"TOUCH",

			"TOUCH key",

			"Returns number of touched keys.<br />" .
			"For compatibility, always return '1'",
			"string",
			"Always return '1'",
			"1.2.17",
			"n/a",
			true,
			null,

			"compat"
	),
);

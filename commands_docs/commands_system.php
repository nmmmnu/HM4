<?php
return array(
	new Cmd(
			"EXIT",

			"EXIT / QUIT",

			"Disconnect from the server.",
			"n/a",
			"n/a",
			"1.0.0",
			"n/a",
			true,
			null,

			"system"
	),

	new Cmd(
			"SHUTDOWN",

			"SHUTDOWN",

			"Shutdowns the server.<br />" .
			"SAVE / NOSAVE is not supported yet.",
			"n/a",
			"n/a",
			"1.0.0",
			"n/a",
			true,
			null,

			"system"
	),

);

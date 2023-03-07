<?php
return array(
	new Cmd(
			"INCR",

			"INCR / INCRBY key [increase value=1]",

			"Atomically increase numerical value of the <i>key</i> with <i>increase value</i>.<br />" .
			"Uses <b>int64_t</b> as a number type.",
			"string (int)",
			"New increased value.",
			"1.1.0",
			"READ + WRITE",
			true,
			true,

			"counter"
	),

	new Cmd(
			"DECR",

			"DECR / DECRBY key [decrease value=1]",

			"Atomically decrease numerical value of the <i>key</i> with <i>decrease value</i>.<br />" .
			"Uses <b>int64_t</b> as a number type.",
			"string (int)",
			"New decrease value.",
			"1.1.0",
			"READ + WRITE",
			true,
			true,

			"counter"
	),
);

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

	new Cmd(
			"INCRTO",

			"INCRTO key value",

			"Atomically increase numerical value of the <i>key</i> to <i>value</i>, but only if new value is greater than old value.<br />" .
			"Uses <b>int64_t</b> as a number type.<br />" .
			"Useful for single heavy hitter.",
			"string (int)",
			"New increased value.",
			"1.3.7.7",
			"READ + WRITE",
			true,
			true,

			"counter"
	),

	new Cmd(
			"DECRTO",

			"DECRTO key value",

			"Atomically decrease numerical value of the <i>key</i> to <i>value</i>, but only if new value is less than old value.<br />" .
			"Uses <b>int64_t</b> as a number type.<br />" .
			"Useful for single heavy hitter.",
			"string (int)",
			"New increased value.",
			"1.3.7.7",
			"READ + WRITE",
			true,
			true,

			"counter"
	),
);

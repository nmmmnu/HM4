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
			"INCRLIMIT",

			"INCRLIMIT key increase_value limit",

			"Atomically increase numerical value of the <i>key</i> with <i>increase_value</i>, but only if new value is less or equal <i>limit</i>.<br />" .
			"Uses <b>int64_t</b> as a number type.<br />" .
			"Useful for financial operations.",
			"bool (int)",
			"1 if limit check is OK, 0 otherwise",
			"1.3.7.8",
			"READ + WRITE",
			false,
			true,

			"counter",

			"<pre>set a 0<br />" .
			"OK<br />" .
			"incrlimit a 8 10<br />" .
			"(integer) 1<br />" .
			"get a<br />" .
			"8<br />" .
			"incrlimit a 8 10<br />" .
			"(integer) 0<br />" .
			"get a<br />" .
			"8</pre>"
	),

	new Cmd(
			"DECRLIMIT",

			"DECRLIMIT key decrease_value limit",

			"Atomically decrease numerical value of the <i>key</i> with <i>decrease_value</i>, but only if new value is greather or equal <i>limit</i>.<br />" .
			"Uses <b>int64_t</b> as a number type.<br />" .
			"Useful for financial operations.",
			"bool (int)",
			"1 if limit check is OK, 0 otherwise",
			"1.3.7.8",
			"READ + WRITE",
			false,
			true,

			"counter",

			"<pre>set a 10<br />" .
			"OK<br />" .
			"decrlimit a 8 0<br />" .
			"(integer) 1<br />" .
			"get a<br />" .
			"2<br />" .
			"decrlimit a 8 0<br />" .
			"(integer) 0<br />" .
			"get a<br />" .
			"2</pre>"
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
			false,
			true,

			"counter",

			"<pre>set a 100<br />" .
			"OK<br />" .
			"incrto a 50<br />" .
			"(integer) 100<br />" .
			"get a<br />" .
			"100<br />" .
			"incrto a 150<br />" .
			"(integer) 150<br />" .
			"get a<br />" .
			"150</pre>"
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
			false,
			true,

			"counter",

			"<pre>set a 100<br />" .
			"OK<br />" .
			"decrto a 150<br />" .
			"(integer) 100<br />" .
			"get a<br />" .
			"100<br />" .
			"decrto a 50<br />" .
			"(integer) 50<br />" .
			"get a<br />" .
			"50</pre>"
	),
);

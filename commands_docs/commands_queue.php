<?php
return array(
	new Cmd(
			"SADD",

			"SADD key val [expiration]",

			"Atomically add <i>value</i> into queue <i>key</i>, with optional expiration of seconds seconds.<br />" .
			"Works exactly as Redis SADD, except internally set key for each value.",
			"OK",
			"OK",
			"1.2.16",
			"WRITE",
			true,
			true,

			"queue"
	),

	new Cmd(
			"SPOP",

			"SPOP key",

			"Atomically remove single element from queue <i>key</i>.<br />" .
			"Works exactly as Redis SPOP, except internally set <b>control key</b> for each queue, also remove the key for the value.",
			"string (int)",
			"Value of the removed element or empty string.",
			"1.2.16",
			"READ + WRITE",
			true,
			true,

			"queue"
	),
);

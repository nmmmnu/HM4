<?php
return array(
	new Cmd(
			"SET",

			"SET key value [seconds=0]",

			"Set <i>key</i> -> <i>value</i> pair, with optional expiration of <i>seconds</i> seconds.",

			"OK",
			"OK",
			"1.0.0",
			"WRITE",
			true,
			true,

			"mutable"
	),

	new Cmd(
			"MSET",

			"MSET key value [key2] [val2]",

			"Set multiple <i>key</i> -> <i>value</i> pairs at once.<br />" .
			"Operation is atomic, so all given keys are set at once.",

			"OK",
			"OK",
			"1.2.17",
			"[number of keys] * WRITE",
			true,
			true,

			"mutable"
	),

	new Cmd(
			"MSETNX",

			"MSETNX key value [key2] [val2]",

			"Set multiple <i>key</i> -> <i>value</i> pairs at once.<br />" .
			"No keys will set, if just a single key already exists.<br />" .
			"Operation is atomic, so all given keys are set at once.",

			"int",
			"0 if some of the key value pair exists.<br />" .
			"1 if none of the key value pair do not exists and keys are set.",
			"1.2.18",
			"[number of keys] * (READ + WRITE)",
			true,
			true,

			"mutable"
	),

	new Cmd(
			"MSETXX",

			"MSETXX key value [key2] [val2]",

			"Set multiple <i>key</i> -> <i>value</i> pairs at once.<br />" .
			"No keys will set, if just a single key does not exists.<br />" .
			"Operation is atomic, so all given keys are set at once.",

			"int",
			"0 if some of the key value pair does not exists.<br />" .
			"1 if none of the key value pair exists and keys are set.",
			"1.2.18",
			"[number of keys] * (READ + WRITE)",
			false,
			true,

			"mutable"
	),

	new Cmd(
			"SETEX",

			"SETEX key seconds value",

			"Set <i>key</i> -> <i>value</i> pair, if key does not exists, with expiration of <i>seconds</i> seconds.<br />" .
			"Doing same as <i>SET key value seconds</i>, but in Redis-compatible way.<br />" .
			"disk.This command is used for PHP session handler.",

			"OK",
			"OK",
			"1.2.11",
			"WRITE",
			true,
			true,

			"mutable"
	),

	new Cmd(
			"SETNX",

			"SETNX key value [seconds=0]",

			"Atomically Set value of the <i>key</i>, if key does not exists, with optional expiration of <i>seconds</i> seconds.<br />" .
			"Note: The command internally GET old key first.",

			"bool",
			"0 if the key value pair exists.<br />" .
			"1 if the key value pair do not exists and is set.",
			"1.2.11",
			"READ + WRITE",
			true,
			true,

			"mutable"
	),

	new Cmd(
			"SETXX",

			"SETXX key value [seconds=0]",

			"Atomically Set value of the <i>key</i>, if key exists, with optional expiration of <i>seconds</i> seconds.<br />" .
			"Note: The command internally GET old key first.",

			"bool",
			"0 if the key value pair exists and is set.<br />" .
			"1 if the key value pair do not exists.",
			"1.2.17",
			"READ + WRITE",
			false,
			true,

			"mutable"
	),

	new Cmd(
			"APPEND",

			"APPEND key value [seconds=0]",

			"Atomically Append value of the <i>key</i>, with optional expiration of <i>seconds</i> seconds.<br />" .
			"Note: The command internally GET old key first.",

			"OK",
			"OK",
			"1.3.0",
			"READ + WRITE",
			true,
			true,

			"mutable"
	),

	new Cmd(
			"EXPIRE",

			"EXPIRE key seconds",

			"Atomically Change the expiration of the <i>key</i> to <i>seconds</i> seconds.<br />" .
			"Note: The command internally GET <i>key</i> first.<br />" .
			"If you can, use SET or SETEX instead.",

			"bool",
			"0 if the key value pair do not exists.<br />" .
			"1 if the key value pair exists and expiration is set.",
			"1.2.11",
			"READ + WRITE",
			true,
			true,

			"mutable"
	),

	new Cmd(
			"PERSIST",

			"PERSIST key seconds",

			"Atomically remove the expiration of the <i>key</i>.<br />" .
			"Note: The command internally GET <i>key</i> first.",

			"bool",
			"0 if the key value pair do not exists.<br />" .
			"1 if the key value pair exists and expiration is removed.",
			"1.2.16",
			"READ + WRITE",
			true,
			true,

			"mutable"
	),

	new Cmd(
			"GETSET",

			"GETSET key value [seconds=0]",

			"Gets the value of the <i>key</i>. Exact match.<br />" .
			"Then atomically Set <i>key</i> -> <i>value</i> pair.<br />" .
			"This command is often used to get value of atomic counter and reset its value to zero.",
			"string",
			"Value of the key or empty string.",
			"1.2.11",
			"READ + WRITE",
			true,
			true,

			"mutable"
	),

	new Cmd(
			"GETDEL",

			"GETDEL key",

			"Gets the value of the <i>key</i>. Exact match.<br />" .
			"Then atomically delete the <i>key</i> -> <i>value</i> pair.<br />",
			"string",
			"Value of the key or empty string.",
			"1.2.16",
			"READ + WRITE",
			true,
			true,

			"mutable"
	),

	new Cmd(
			"DEL",

			"DEL key [key2]... / UNLINK key [key2]...",

			"Removes <i>key</i>, <i>key2</i>...",
			"bool",
			"Always return 1",
			"1.0.0",
			"[number of keys] * WRITE",
			true,
			true,

			"mutable"
	),
);

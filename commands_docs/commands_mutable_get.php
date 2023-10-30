<?php
return array(
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

			"mutable_get"
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

			"mutable_get"
	),

	new Cmd(
			"GETEX",

			"GETEX key seconds",

			"Gets the value of the <i>key</i>. Exact match.<br />" .
			"Then atomically Change the expiration of the <i>key</i> to <i>seconds</i> seconds.",
			"string",
			"Value of the key or empty string.",
			"1.2.16",
			"READ + WRITE",
			false,
			true,

			"mutable_get"
	),

	new Cmd(
			"GETEXAT",

			"GETEXAT key timestamp",

			"Gets the value of the <i>key</i>. Exact match.<br />" .
			"Then atomically Change the expiration of the <i>key</i>, so <i>key</i> expire at specific timestamp.<br />" .
			"If timestamp is in the past, the <i>key</i> is deleted.",
			"string",
			"Value of the key or empty string.",
			"1.2.16",
			"READ + WRITE",
			false,
			true,

			"mutable_get"
	),

	new Cmd(
			"GETPERSIST",

			"GETPERSIST key",

			"Gets the value of the <i>key</i>. Exact match.<br />" .
			"Then atomically remove the expiration of the <i>key</i>.",
			"string",
			"Value of the key or empty string.",
			"1.2.16",
			"READ + WRITE",
			false,
			true,

			"mutable_get"
	),

);

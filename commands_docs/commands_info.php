<?php
return array(
	new Cmd(
			"INFO",

			"INFO",

			"Returns server information.",
			"string",
			"Server information.",
			"1.0.0",
			"n/a",
			true,
			null,

			"info"
	),

	new Cmd(
			"DBSIZE",

			"DBSIZE",

			"Returns number of keys.",
			"string (int)",
			"number of keys",
			"1.2.18",
			"n/a",
			true,
			null,

			"info"
	),

	new Cmd(
			"VERSION",

			"VERSION",

			"Returns server version.",
			"string",
			"Server version.",
			"1.2.16",
			"n/a",
			false,
			null,

			"info"
	),

	new Cmd(
			"MAXKEYSIZE",

			"MAXKEYSIZE",

			"Returns MAX_KEY_SIZE.",
			"int",
			"max key size",
			"1.2.30",
			"n/a",
			false,
			null,

			"info"
	),

	new Cmd(
			"MAXVALSIZE",

			"MAXVALSIZE",

			"Returns MAX_VAL_SIZE.",
			"int",
			"max value size",
			"1.2.30",
			"n/a",
			false,
			null,

			"info"
	),

	new Cmd(
			"PING",

			"PING",

			"Returns PONG",
			"string",
			"pong",
			"1.2.16",
			"n/a",
			true,
			null,

			"info"
	),

	new Cmd(
			"ECHO",

			"ECHO message",

			"Returns <i>message</i>.",
			"string",
			"the message",
			"1.2.16",
			"n/a",
			true,
			null,

			"info"
	),

	new Cmd(
			"TIME",

			"TIME",

			"Returns current time as timestamp, redis style.",
			"array",
			"first element - current timestamp. second element current microseconds",
			"1.3.7.5",
			"n/a",
			true,
			null,

			"info"
	),
);

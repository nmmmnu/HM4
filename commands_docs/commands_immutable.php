<?php
return array(
	new Cmd(
			"GET",

			"GET key",

			"Get value of the <i>key</i>. Exact match.",

			"string",
			"Value of the key or empty string.",
			"1.0.0",
			"READ",
			true,
			false,

			"immutable"
	),

	new Cmd(
			"MGET",

			"MGET key1 [key2]...",

			"Get value of the <i>key1</i>, <i>key2</i>... Exact match.<br />" .
			"Same as GET but array responce.",

			"array",
			"Value of the key or empty string.",
			"1.0.0",
			"[number of keys] * READ",
			true,
			false,

			"immutable"
	),

	new Cmd(
			"EXISTS",

			"EXISTS key",

			"Get information if <i>key</i> exists. Exact match.<br />" .
			"Operation is not recommended, because it is as fast as if you get the <i>key</i> itself.",

			"bool",
			"0 if the key value pair do not exists.<br />" .
			"1 if the key value pair exists.",
			"1.2.16",
			"READ",
			true,
			false,

			"immutable"
	),

	new Cmd(
			"TTL",

			"TTL key",

			"Get TTL value of the <i>key</i>. Exact match.",

			"int",
			"Value of the TTL or 0, if there is no expiration set.",

			"1.2.11",
			"READ",
			true,
			false,

			"immutable"
	),

	new Cmd(
			"EXPIRETIME",

			"EXPIRETIME key",

			"Get timestamp when  <i>key</i> will expire. Exact match.",

			"int",
			"Timestamp or 0, if there is no expiration set.",

			"1.2.11",
			"READ",
			true,
			false,

			"immutable"
	),

	new Cmd(
			"DUMP",

			"DUMP key",

			"Get internal <i>key</i> representation. Exact match.<br />" .
			"Works on deleted and expired keys too.<br />." .
			"Not compatible with Redis",

			"string",
			"internal <i>key</i> representation",

			"1.3.4",
			"READ",
			false,
			false,

			"immutable"
	),

	new Cmd(
			"GETRANGE",

			"GETRANGE key index_start index_finish",

			"Get substring of the value of the <i>key</i> from <i>index_start</i> to <i>index_finish</i>. Exact match.<br />" .
			"Unlike Redis, negative indexes are not supported.",

			"string",
			"substring of the value or empty string.",

			"1.2.30",
			"READ",
			true,
			false,

			"immutable"
	),

	new Cmd(
			"STRLEN",

			"STRLEN / SIZE key",

			"Get size of the value of the <i>key</i>. Exact match.<br />" .
			"Useful for debuging HLL, BF and BIT commands.",

			"int",
			"Size of the value.",

			"1.2.17",
			"READ",
			true,
			false,

			"immutable"
	),
);

<?php
return array(
	new Cmd(
			"HGETALL",

			"HGETALL key",

			"Get <i>key</i> hash.<br />" .
			"Returns up to 1'000 elements.<br />" .
			"Works exactly as Redis HGETALL, except internally set key for each value.",
			"array",
			"array of subkey and values from key hash",
			"1.2.17",
			"READ",
			true,
			true,

			"immutable_x"
	),



	new Cmd(
			"HGETKEYS",

			"HGETKEYS key",

			"Get <i>key</i> hash.<br />" .
			"Returns up to 1'000 elements.<br />" .
			"Works exactly as Redis HGETALL, except internally set key for each value.",
			"array",
			"array of subkey from key hash",
			"1.2.19",
			"READ",
			true,
			true,

			"immutable_x"
	),



	new Cmd(
			"HGETVALS",

			"HGETVALS key",

			"Get <i>key</i> hash.<br />" .
			"Returns up to 1'000 elements.<br />" .
			"Works exactly as Redis HGETVALS, except internally set key for each value.",
			"array",
			"array of values from key hash",
			"1.2.19",
			"READ",
			true,
			true,

			"immutable_x"
	),



	new Cmd(
			"HLEN",

			"HLEN key",

			"Get <i>key</i> hash.<br />" .
			"Counts up to 1'000 elements.<br />" .
			"Works exactly as Redis HLEN, except internally set key for each value.",
			"int",
			"number of elements in key hash",
			"1.2.19",
			"READ",
			true,
			true,

			"immutable_x"
	),



	new Cmd(
			"HGET",

			"HGET key subkey",

			"Get <i>key</i> -> <i>subkey</i> hash.<br />" .
			"Works exactly as Redis HGET, except internally set key for each value.",
			"string",
			"Value of the key -> subkey hash or empty string.",
			"1.2.17",
			"READ",
			true,
			true,

			"immutable"
	),



	new Cmd(
			"HMGET",

			"HMGET key subkey [subkey1]...",

			"Get <i>key</i> -> <i>subkey</i>, <i>subkey1</i>... hash.<br />" .
			"Works exactly as Redis HMGET, except internally set key for each value.<br />" .
			"If you can, use HGETALL it is much faster.",
			"array",
			"Value of the key -> subkey hash or empty string.",
			"1.2.17",
			"[number of keys] * READ",
			true,
			true,

			"immutable"
	),



	new Cmd(
			"HEXISTS",

			"HEXISTS key subkey",

			"Get information if <i>key</i> -> <i>subkey</i> hash exists.<br /><br />" .
			"Works exactly as Redis HSET, except internally set key for each value.<br />" .
			"Operation is not recommended, because it is as fast as if you get the hash itself.",
			"bool",
			"0 if the key -> subkey hash do not exists.<br />" .
			"1 if the key -> subkey hash exists.",
			"1.2.17",
			"READ",
			true,
			true,

			"immutable"
	),



	new Cmd(
			"HSET",

			"HSET key subkey val [seconds]",

			"Set <i>key</i> -> <i>subkey</i> -> <i>value</i> hash, with optional expiration of <i>seconds</i> seconds.<br />" .
			"Works exactly as Redis HSET, except internally set key for each value.",
			"OK",
			"OK",
			"1.2.17",
			"WRITE",
			true,
			true,

			"mutable"
	),



	new Cmd(
			"HDEL",

			"HDEL key subkey [subkey2]...",

			"Removes <i>key</i> -> <i>subkey</i>, <i>key</i> -> <i>subkey2</i>... hash.<br />" .
			"Works exactly as Redis HSET, except internally set key for each value.",
			"bool",
			"Always return 1",
			"1.2.17",
			"[number of keys] * WRITE",
			true,
			true,

			"mutable"
	),



	new Cmd(
			"HDELALL",

			"HDELALL key",

			"Delete up to 10'000 key-value pairs from hash <i>key</i>.<br />" .
			"Works exactly as Redis DEL [hash key].",
			"bool",
			"1 if all keys are removed, 0 if keys remains",
			"1.3.4",
			"[number of keys] * WRITE",
			true,
			true,

			"mutable_x"
	),



	new Cmd(
			"HPERSISTALL",

			"HPERSISTALL key",

			"PERSIST up to 10'000 key-value pairs from hash <i>key</i>.<br />" .
			"Works exactly as Redis PERSIST [hash key].",
			"bool",
			"1 if all keys are removed, 0 if keys remains",
			"1.3.4",
			"[number of keys] * WRITE",
			true,
			true,

			"mutable_x"
	),



	new Cmd(
			"HEXPIREALL",

			"HEXPIREALL key",

			"EXPIRE up to 10'000 key-value pairs from hash <i>key</i>.<br />" .
			"Works exactly as Redis EXPIRE [hash key].",
			"bool",
			"1 if all keys are removed, 0 if keys remains",
			"1.3.4",
			"[number of keys] * WRITE",
			true,
			true,

			"mutable_x"
	),
);

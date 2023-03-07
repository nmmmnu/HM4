<?php
return array(
	new Cmd(
			"COPY",

			"COPY old_key new_key",

			"Atomically copy the <i>old_key</i> to the <i>new_key</i>.<br />" .
			"Note: The command internally GET <i>old_key</i> first.",

			"bool",
			"0 if the key value pair do not exists.<br />" .
			"1 if the key value pair exists and name is changed.",
			"1.2.16",
			"READ + WRITE",
			true,
			true,

			"copy"
	),

	new Cmd(
			"COPYNX",

			"COPYNX old_key new_key",

			"Atomically copy the <i>old_key</i> to the <i>new_key</i>, if <i>new_key</i> does not exists.<br />" .
			"Note: The command internally GET <i>old_key</i> and <i>new_key</i> first.<br />" .
			"Note: Redis does not support this operation.",

			"bool",
			"0 if the key value pair do not exists.<br />" .
			"1 if the key value pair exists and name is changed.",
			"1.2.16",
			"2 * READ + WRITE",
			false,
			true,

			"copy"
	),

	new Cmd(
			"RENAME",

			"RENAME old_key new_key / MOVE old_key new_key",

			"Atomically renames <i>old_key</i> to <i>new_key</i>.",

			"bool",
			"0 if the key value pair do not exists.<br />" .
			"1 if the key value pair exists and name is changed.",
			"1.2.16",
			"READ + WRITE",
			true,
			true,

			"copy"
	),

	new Cmd(
			"RENAMENX",

			"RENAMENX old_key new_key / MOVENX old_key new_key",

			"Atomically renames <i>old_key</i> to <i>new_key</i>, if <i>new_key</i> does not exists.<br />" .
			"Note: The command internally GET <i>old_key</i> and <i>new_key</i> first." ,

			"bool",
			"0 if the key value pair do not exists.<br />" .
			"1 if the key value pair exists and name is changed.",
			"1.2.16",
			"2 * READ + WRITE",
			true,
			true,

			"copy"
	),
);

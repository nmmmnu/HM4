<?php
return array(
	new Cmd(
			"DELX",

			"DELX key prefix",

			"Delete up to 10'000 key-value pairs after <i>key</i>.<br />" .
			"Delete ONLY valid pairs, and only if they are matching the <i>prefix</i>.<br />" .
			"<br />" .
			"<u>Example:</u><br />" .
			"<br />" .
			"<pre>set u:001:name  John<br />" .
			"set u:001:city  LA<br />" .
			"set u:001:state CA<br />" .
			"set u:001:phone 1.800.12345678<br />" .
			"getx u:001: 1000 u:001:<br />" .
			"delx u:001:      u:001:</pre>",

			"string",
			"Last key, if there is second page.",

			"1.2.17",
			"READ + [number of keys] * WRITE",
			false,
			true,

			"mutable_x"
	),



	new Cmd(
			"PERSISTX",

			"PERSISTX key prefix",

			"PERSIST up to 10'000 key-value pairs after <i>key</i>.<br />" .
			"PERSIST ONLY valid pairs, and only if they are matching the <i>prefix</i>.<br />" .
			"<br />" .
			"<u>Example:</u><br />" .
			"<br />" .
			"<pre>set u:001:name  John<br />" .
			"set u:001:city  LA<br />" .
			"set u:001:state CA<br />" .
			"set u:001:phone 1.800.12345678<br />" .
			"getx u:001: 1000 u:001:<br />" .
			"persistx u:001: u:001:</pre>",

			"string",
			"Last key, if there is second page.",

			"1.2.17",
			"READ + [number of keys] * WRITE",
			false,
			true,

			"mutable_x"
	),



	new Cmd(
			"EXPIREX",

			"EXPIREX key expiration prefix",

			"EXPIRE up to 10'000 key-value pairs after <i>key</i>.<br />" .
			"EXPIRE ONLY valid pairs, and only if they are matching the <i>prefix</i>.<br />" .
			"<br />" .
			"<u>Example:</u><br />" .
			"<br />" .
			"<pre>set u:001:name  John<br />" .
			"set u:001:city  LA<br />" .
			"set u:001:state CA<br />" .
			"set u:001:phone 1.800.12345678<br />" .
			"expires u:001: 90 u:001:</pre>",

			"string",
			"Last key, if there is second page.",

			"1.2.17",
			"READ + [number of keys] * WRITE",
			false,
			true,

			"mutable_x"
	),
);

<?php
return array(
	new Cmd(
			"GETX",

			"GETX key number prefix",

			"Gets <i>number</i> of key-value pairs after <i>key</i>.<br />" .
			"Returns ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Returns up to 1'000 elements.<br />" .
			"<br />" .
			"<u>Example:</u><br />" .
			"<br />" .
			"<pre>set u:001:name  John<br />" .
			"set u:001:city  LA<br />" .
			"set u:001:state CA<br />" .
			"set u:001:phone 1.800.12345678<br />" .
			"getx u:001: 1000 u:001:</pre>",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.0.0",
			"READ",
			false,
			false,

			"immutable_x"
	),
);


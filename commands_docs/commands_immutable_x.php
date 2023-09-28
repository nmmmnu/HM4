<?php
return array(
	new Cmd(
			"XNGET",

			"XNGET / GETX key number prefix",

			"Gets <i>number</i> of key-value pairs after <i>key</i>.<br />" .
			"Returns ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Returns up to 1'000 elements.<br />" .
			"<br />" .
			"<b>This command is similar to following MySQL statement:</b><br />" .
			"<pre>select key, val from table where key >= [key] and key like '[key]%' limit [number]</pre>",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.0.0",
			"READ",
			false,
			false,

			"immutable_x",

			"<pre>set u:001:name  John<br />" .
			"set u:001:city  LA<br />" .
			"set u:001:state CA<br />" .
			"set u:001:phone 1.800.12345678<br />" .
			"<br />" .
			"xnget u:001: 1000 u:001:</pre>"

	),
	new Cmd(
			"XRGET",

			"XRGET key number range_end",

			"Gets <i>number</i> of key-value pairs after <i>key</i>.<br />" .
			"Returns ONLY valid pairs, but only if they are less than or equal <i>range_end</i>.<br />" .
			"Returns up to 1'000 elements.<br />" .
			"<br />" .
			"<b>This command is similar to following MySQL statement:</b><br />" .
			"<pre>select key, val from table where key >= [key] and key < [range_end] limit [number]</pre>",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"immutable_x",

			"<pre>set price:2010-01 5<br />" .
			"set price:2010-02 6<br />" .
			"set price:2010-03 10<br />" .
			"set price:2010-04 16<br />" .
			"set price:2010-05 22<br />" .
			"<br />" .
			"xrget price:2010-02 1000 price:2010-04</pre>"

	),
	new Cmd(
			"XUGET",

			"XUGET key number",

			"Gets <i>number</i> of key-value pairs after <i>key</i>.<br />" .
			"Returns ONLY valid pairs, but unlike XNGET and XRGET range is unbounded.<br />" .
			"Useful for database dump.<br />" .
			"Returns up to 1'000 elements.<br />" .
			"<br />" .
			"<b>This command is similar to following MySQL statement:</b><br />" .
			"<pre>select key, val from table where key >= [key] limit [number]</pre>",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"immutable_x",

			"<pre>xrget '' 1000</pre>"

	),
);


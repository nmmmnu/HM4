<?php
return array(
	new Cmd(
			"XNGET",

			"XNGET / GETX key number prefix",

			"Gets <i>number</i> of key-value pairs after <i>key</i>.<br />" .
			"Returns ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Returns up to ~32'000 elements.",

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
			"xnget u:001: 1000 u:001:</pre>",

			"<pre>select key, val from table where key >= [key] and key like '[prefix]%' limit [number]</pre>"
	),
	new Cmd(
			"XRGET",

			"XRGET key number range_end",

			"Gets <i>number</i> of key-value pairs after <i>key</i>.<br />" .
			"Returns ONLY valid pairs, but only if they are less than or equal <i>range_end</i>.<br />" .
			"Returns up to ~32'000 elements.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"immutable_x",

			"<pre>set price:2010-01  5<br />" .
			"set price:2010-02  6<br />" .
			"set price:2010-03 10<br />" .
			"set price:2010-04 16<br />" .
			"set price:2010-05 22<br />" .
			"<br />" .
			"xrget price:2010-02 1000 price:2010-04</pre>",

			"<pre>select key, val from table where key >= [key] and key <= [range_end] limit [number]</pre>"
	),
	new Cmd(
			"XUGET",

			"XUGET key number",

			"Gets <i>number</i> of key-value pairs after <i>key</i>.<br />" .
			"Returns ONLY valid pairs, but unlike XNGET and XRGET range is unbounded.<br />" .
			"Useful for database dump.<br />" .
			"Returns up to ~32'000 elements.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"immutable_x",

			"<pre>xuget '' 1000</pre>",

			"<pre>select key, val from table where key >= [key] limit [number]</pre>"
	),



	new Cmd(
			"XNGETKEYS",

			"XNGETKEYS key number prefix",

			"Same as XNGET, but instead of values, return 1.<br>" .
			"1 is returned because some languages similar to php, may skip the values.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.0.0",
			"READ",
			false,
			false,

			"immutable_x"
	),
	new Cmd(
			"XRGETKEYS",

			"XRGETKEYS key number range_end",

			"Same as XRGET, but instead of values, return 1.<br>" .
			"1 is returned because some languages similar to php, may skip the values.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"immutable_x"
	),
	new Cmd(
			"XUGETKEYS",

			"XUGETKEYS key number",

			"Same as XUGET, but instead of values, return 1.<br>" .
			"1 is returned because some languages similar to php, may skip the values.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"immutable_x"
	),



	new Cmd(
			"XNNEXT",

			"XNNEXT key prefix",

			"Gets one key-value pair after <i>key</i>.<br />" .
			"Returns ONLY valid pairs, but only if they are matching the <i>prefix</i>.",

			"array",
			"key or empty<br />" .
			"value or last key, if there is second page.",

			"1.3.7.8",
			"READ",
			false,
			false,

			"immutable_x",

			"<pre>set a:2001  20<br />" .
			"set a:2002  30<br />" .
			"set a:2003  10<br />" .
			"set a:2004  50<br />" .
			"<br />" .
			"xnnext a: a:</pre>",

			"<pre>select key, val from table where key > [key] and key like '[prefix]%' limit 1</pre>"
	),
	new Cmd(
			"XRNEXT",

			"XRNEXT key range_end",

			"Gets one key-value pair after <i>key</i>.<br />" .
			"Returns ONLY valid pairs, but only if they are less than or equal <i>range_end</i>.",

			"array",
			"key or empty<br />" .
			"value or last key, if there is second page.",

			"1.3.7.8",
			"READ",
			false,
			false,

			"immutable_x",

			"<pre>set a:2001  20<br />" .
			"set a:2002  30<br />" .
			"set a:2003  10<br />" .
			"set a:2004  50<br />" .
			"<br />" .
			"xnnext a: a:2002</pre>",

			"<pre>select key, val from table where key > [key] and key <= [range_end] limit 1</pre>"
	),
	new Cmd(
			"XUNEXT",

			"XUNEXT key",

			"Gets one key-value pair after <i>key</i>.<br />" .
			"Returns ONLY valid pairs, but unlike XNGET and XRGET range is unbounded.<br />" .
			"For database dump use XUGET.",

			"array",
			"key or empty<br />" .
			"value or last key, if there is second page.",

			"1.3.7.8",
			"READ",
			false,
			false,

			"immutable_x",

			"<pre>xunext ''</pre>",

			"<pre>select key, val from table where key > [key] limit 1</pre>"
	),
);


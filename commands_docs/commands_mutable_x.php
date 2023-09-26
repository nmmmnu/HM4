<?php
return array(
	new Cmd(
			"DELX",

			"DELX key prefix",

			"Delete up to 10'000 key-value pairs after <i>key</i>.<br />" .
			"Delete ONLY valid pairs, and only if they are matching the <i>prefix</i>.",

			"string",
			"Last key, if there is second page.",

			"1.2.17",
			"READ + [number of keys] * WRITE",
			false,
			true,

			"mutable_x",

			"<pre>set u:001:name  John<br />" .
			"set u:001:city  LA<br />" .
			"set u:001:state CA<br />" .
			"set u:001:phone 1.800.12345678<br />" .
			"<br />" .
			"getx u:001: 1000 u:001:<br />" .
			"delx u:001:      u:001:</pre>"
	),



	new Cmd(
			"DELXR",

			"DELXR key range_end",

			"Delete up to 10'000 key-value pairs after <i>key</i>.<br />" .
			"Delete ONLY valid pairs, and only if they are less than or equal <i>range_end</i>.",

			"string",
			"Last key, if there is second page.",

			"1.3.7.1",
			"READ + [number of keys] * WRITE",
			false,
			true,

			"mutable_x",

			"<pre>set price:2010-01 5<br />" .
			"set price:2010-02 6<br />" .
			"set price:2010-03 10<br />" .
			"set price:2010-04 16<br />" .
			"set price:2010-05 22<br />" .
			"<br />" .
			"getxr price:2010-02 1000 price:2010-04<br />" .
			"delxr price:2010-02      price:2010-04</pre>"
	),



	new Cmd(
			"PERSISTX",

			"PERSISTX key prefix",

			"PERSIST up to 10'000 key-value pairs after <i>key</i>.<br />" .
			"PERSIST ONLY valid pairs, and only if they are matching the <i>prefix</i>.",

			"string",
			"Last key, if there is second page.",

			"1.2.17",
			"READ + [number of keys] * WRITE",
			false,
			true,

			"mutable_x",

			"<pre>set u:001:name  John<br />" .
			"set u:001:city  LA<br />" .
			"set u:001:state CA<br />" .
			"set u:001:phone 1.800.12345678<br />" .
			"<br />" .
			"getx     u:001: 1000 u:001:<br />" .
			"persistx u:001:      u:001:</pre>"
	),



	new Cmd(
			"PERSISTXR",

			"PERSISTXR key prefix",

			"PERSIST up to 10'000 key-value pairs after <i>key</i>.<br />" .
			"PERSIST ONLY valid pairs, and only if they are less than or equal the <i>range_end</i>.",

			"string",
			"Last key, if there is second page.",

			"1.3.7.1",
			"READ + [number of keys] * WRITE",
			false,
			true,

			"mutable_x",

			"<pre>set price:2010-01 5<br />" .
			"set price:2010-02 6<br />" .
			"set price:2010-03 10<br />" .
			"set price:2010-04 16<br />" .
			"set price:2010-05 22<br />" .
			"<br />" .
			"getxr     price:2010-02 1000 price:2010-04<br />" .
			"persistxr price:2010-02      price:2010-04</pre>"
	),



	new Cmd(
			"EXPIREX",

			"EXPIREX key expiration prefix",

			"EXPIRE up to 10'000 key-value pairs after <i>key</i>.<br />" .
			"EXPIRE ONLY valid pairs, and only if they are matching the <i>prefix</i>.",

			"string",
			"Last key, if there is second page.",

			"1.2.17",
			"READ + [number of keys] * WRITE",
			false,
			true,

			"mutable_x",

			"<pre>set u:001:name  John<br />" .
			"set u:001:city  LA<br />" .
			"set u:001:state CA<br />" .
			"set u:001:phone 1.800.12345678<br />" .
			"<br />" .
			"getx    u:001: 1000 u:001:<br />" .
			"expirex u:001:   90 u:001:</pre>"
	),



	new Cmd(
			"EXPIREXR",

			"EXPIREX key expiration prefix",

			"EXPIRE up to 10'000 key-value pairs after <i>key</i>.<br />" .
			"EXPIRE ONLY valid pairs, and only if they are less than or equal the <i>range_end</i>.",

			"string",
			"Last key, if there is second page.",

			"1.3.7.1",
			"READ + [number of keys] * WRITE",
			false,
			true,

			"mutable_x",

			"<pre>set price:2010-01 5<br />" .
			"set price:2010-02 6<br />" .
			"set price:2010-03 10<br />" .
			"set price:2010-04 16<br />" .
			"set price:2010-05 22<br />" .
			"<br />" .
			"getxr   price:2010-02 1000 price:2010-04<br />" .
			"expirex price:2010-02   90 price:2010-04</pre>"
	),
);


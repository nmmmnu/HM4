<?php
return array(
	new Cmd(
			"XNDEL",

			"XNDEL key prefix",

			"Delete up to 65'536 key-value pairs after <i>key</i>.<br />" .
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
			"xnget u:001: 1000 u:001:<br />" .
			"xndel u:001:      u:001:</pre>",

			"<pre>delete from table where key >= [key] and key like '[key]%' limit 65'536</pre>"
	),



	new Cmd(
			"XRDEL",

			"XRDEL key range_end",

			"Delete up to 65'536 key-value pairs after <i>key</i>.<br />" .
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
			"xrget price:2010-02 1000 price:2010-04<br />" .
			"xrdel price:2010-02      price:2010-04</pre>",

			"<pre>delete from table where key >= [key] and key < [range_end] limit 65'536</pre>"
	),



	new Cmd(
			"XNPERSIST",

			"XNPERSIST key prefix",

			"Persist up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Persist ONLY valid pairs, and only if they are matching the <i>prefix</i>.",

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
			"xnget     u:001: 1000 u:001:<br />" .
			"xnpersist u:001:      u:001:</pre>",

			"<pre>update table set expire = 0 where key >= [key] and key like '[key]%' limit 65'536</pre>"
	),



	new Cmd(
			"XRPERSIST",

			"XRPERSIST key prefix",

			"PERSIST up to 65'536 key-value pairs after <i>key</i>.<br />" .
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
			"xnget     price:2010-02 1000 price:2010-04<br />" .
			"xnpersist price:2010-02      price:2010-04</pre>",

			"<pre>update table set expire = 0 where key >= [key] and key < [range_end] limit 65'536</pre>"
	),



	new Cmd(
			"XNEXPIRE",

			"XNEXPIRE key expiration prefix",

			"EXPIRE up to 65'536 key-value pairs after <i>key</i>.<br />" .
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
			"xnget    u:001: 1000 u:001:<br />" .
			"xnexpire u:001:   90 u:001:</pre>",

			"<pre>update table set expire = [expiration] where key >= [key] and key like '[key]%' limit 65'536</pre>"
	),



	new Cmd(
			"XREXPIRE",

			"XREXPIRE key expiration prefix",

			"EXPIRE up to 65'536 key-value pairs after <i>key</i>.<br />" .
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
			"xrget    price:2010-02 1000 price:2010-04<br />" .
			"xrexpire price:2010-02   90 price:2010-04</pre>",

			"<pre>update table set expire = [expiration] where key >= [key] and key < [range_end] limit 65'536</pre>"
	),



	new Cmd(
			"XNEXPIREAT",

			"XNEXPIREAT key timestamp prefix",

			"EXPIREAT up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"EXPIREAT ONLY valid pairs, and only if they are matching the <i>prefix</i>.",

			"string",
			"Last key, if there is second page.",

			"1.3.7.5",
			"READ + [number of keys] * WRITE",
			false,
			true,

			"mutable_x",

			"<pre>set u:001:name  John<br />" .
			"set u:001:city  LA<br />" .
			"set u:001:state CA<br />" .
			"set u:001:phone 1.800.12345678<br />" .
			"<br />" .
			"xnget      u:001:       1000 u:001:<br />" .
			"xnexpireat u:001: 1830290400 u:001:</pre>",

			"<pre>update table set expire_at = [expiration] where key >= [key] and key like '[key]%' limit 65'536</pre>"
	),



	new Cmd(
			"XREXPIREAT",

			"XREXPIREAT key timestamp prefix",

			"EXPIREAT up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"EXPIREAT ONLY valid pairs, and only if they are less than or equal the <i>range_end</i>.",

			"string",
			"Last key, if there is second page.",

			"1.3.7.5",
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
			"xrget      price:2010-02       1000 price:2010-04<br />" .
			"xrexpireat price:2010-02 1830290400 price:2010-04</pre>",

			"<pre>update table set expire_at = [expiration] where key >= [key] and key < [range_end] limit 65'536</pre>"
	),
);


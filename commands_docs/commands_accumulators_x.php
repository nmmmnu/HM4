<?php
return array(
	new Cmd(
			"COUNT",

			"COUNT key number prefix",

			"Accumulate using COUNT <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Accumulate up to 65'536 elements.",


			"array",
			"First element  - count of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.4",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set dom:google:google.com  some_data<br />" .
			"set dom:google:youtube.com some_data<br />" .
			"set dom:google:gmail.com   some_data<br />" .
			"set dom:google:blogger.com some_data<br />" .
			"set dom:google:abc.xyz     some_data<br />" .
			"<br />" .
			"getx    dom:google: 1000 dom:google:<br />" .
			"count   dom:google: 1000 dom:google:</pre>",

			"<pre>select count(*) from table where key >= [key] and key like '[key]%' limit [number]</pre>"
	),

	new Cmd(
			"SUM",

			"SUM key number prefix",

			"Accumulate using SUM <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Accumulate up to 65'536 elements.",

			"array",
			"First element  - sum of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.4",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"getx visits:202001 1000 visits:202001<br />" .
			"sum  visits:202001 1000 visits:202001<br />" .
			"<br />" .
			"getx visits:2020   1000 visits:2020<br />" .
			"sum  visits:2020   1000 visits:2020</pre>",

			"<pre>select sum(val) from table where key >= [key] and key like '[key]%' limit [number]</pre>"
	),

	new Cmd(
			"XNCOUNT",

			"XNCOUNT key prefix",

			"Accumulate using COUNT up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.",

			"array",
			"First element  - count of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.4",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set dom:google:google.com  some_data<br />" .
			"set dom:google:youtube.com some_data<br />" .
			"set dom:google:gmail.com   some_data<br />" .
			"set dom:google:blogger.com some_data<br />" .
			"set dom:google:abc.xyz     some_data<br />" .
			"<br />" .
			"xnget   dom:google: 1000 dom:google:<br />" .
			"xncount dom:google:      dom:google:</pre>",

			"<pre>select count(*) from table where key >= [key] and key like '[key]%' limit 65'536</pre>"
	),

	new Cmd(
			"XRCOUNT",

			"XRCOUNT key range_end",

			"Accumulate using COUNT up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.",

			"array",
			"First element  - count of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set dom:google:google.com  some_data<br />" .
			"set dom:google:youtube.com some_data<br />" .
			"set dom:google:gmail.com   some_data<br />" .
			"set dom:google:blogger.com some_data<br />" .
			"set dom:google:abc.xyz     some_data<br />" .
			"<br />" .
			"xrget   dom:google: 1000 dom:google:blogger.com<br />" .
			"xrcount dom:google:      dom:google:blogger.com</pre>",

			"<pre>select count(*) from table where key >= [key] and key < [range_end] limit 65'536</pre>"
	),

	new Cmd(
			"XNSUM",

			"XNSUM key prefix",

			"Accumulate using SUM up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.",

			"array",
			"First element  - sum of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.4",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"xnget visits:202001 1000 visits:202001<br />" .
			"xnsum visits:202001      visits:202001<br />" .
			"<br />" .
			"xnget visits:2020   1000 visits:2020<br />" .
			"xnsum visits:2020        visits:2020</pre>",

			"<pre>select sum(val) from table where key >= [key] and key like '[key]%' limit 65'536</pre>"
	),

	new Cmd(
			"XRSUM",

			"XRSUM key range_end",

			"Accumulate using SUM up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.",

			"array",
			"First element  - sum of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"xrget visits:202001 1000 visits:202003<br />" .
			"xrsum visits:202001      visits:202003</pre>",

			"<pre>select sum(val) from table where key >= [key] and key < [range_end] limit 65'536</pre>"
	),

	new Cmd(
			"XNMIN",

			"XNMIN key prefix",

			"Accumulate using MIN up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.",

			"array",
			"First element  - min of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.5",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"xnget visits:202001 1000 visits:202001<br />" .
			"xnmin visits:202001      visits:202001<br />" .
			"<br />" .
			"xnget visits:2020   1000 visits:2020<br />" .
			"xnmin visits:2020        visits:2020</pre>",

			"<pre>select min(val) from table where key >= [key] and key like '[key]%' limit 65'536</pre>"
	),

	new Cmd(
			"XRMIN",

			"XRMIN key range_end",

			"Accumulate using MIN up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.",

			"array",
			"First element  - min of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"xrget visits:202001 1000 visits:202003<br />" .
			"xrmin visits:202001      visits:202003</pre>",

			"<pre>select min(val) from table where key >= [key] and key < [range_end] limit 65'536</pre>"
	),

	new Cmd(
			"XNMAX",

			"XNMAX key prefix",

			"Accumulate using MAX up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.",

			"array",
			"First element  - max of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.5",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"xnget visits:202001 1000 visits:202001<br />" .
			"xnmax visits:202001      visits:202001<br />" .
			"<br />" .
			"xnget visits:2020   1000 visits:2020<br />" .
			"xnmax visits:2020        visits:2020</pre>",

			"<pre>select max(val) from table where key >= [key] and key like '[key]%' limit 65'536</pre>"
	),

	new Cmd(
			"XRMAX",

			"XRMAX key prefix",

			"Accumulate using MAX up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.",

			"array",
			"First element  - max of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"xrget visits:202001 1000 visits:202003<br />" .
			"xrmax visits:202001      visits:202003</pre>",

			"<pre>select max(val) from table where key >= [key] and key < [range_end] limit 65'536</pre>"
	),

	new Cmd(
			"XNFIRST",

			"XNFIRST key prefix",

			"Accumulate using FIRST up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.",

			"array",
			"First element  - max of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.5",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"xnget   visits:202001 1000 visits:202001<br />" .
			"xnfirst visits:202001      visits:202001<br />" .
			"<br />" .
			"xnget   visits:2020   1000 visits:2020<br />" .
			"xnfirst visits:2020        visits:2020</pre>",

			"<pre>select first(val) from table where key >= [key] and key like '[key]%' limit 65'536</pre>" .
			"(Note MySQL does not support first(), but PostgreSQL does)"
	),

	new Cmd(
			"XRFIRST",

			"XRFIRST key prefix",

			"Accumulate using FIRST up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.",

			"array",
			"First element  - max of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"xrget visits:202001 1000 visits:202003<br />" .
			"xrmax visits:202001      visits:202003</pre>",

			"<pre>select first(val) from table where key >= [key] and key < [range_end] limit 65'536</pre>" .
			"(Note MySQL does not support first(), but PostgreSQL does)"
	),

	new Cmd(
			"XNLAST",

			"XNLAST key prefix",

			"Accumulate using LAST up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.",

			"array",
			"First element  - max of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.5",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"xnget  visits:202001 1000 visits:202001<br />" .
			"xnlast visits:202001      visits:202001<br />" .
			"<br />" .
			"xnget  visits:2020   1000 visits:2020<br />" .
			"xnlast visits:2020        visits:2020</pre>",

			"<pre>select last(val) from table where key >= [key] and key like '[key]%' limit 65'536</pre>" .
			"(Note MySQL does not support last(), but PostgreSQL does)"
	),

	new Cmd(
			"XRLAST",

			"XRLAST key prefix",

			"Accumulate using LAST up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.",

			"array",
			"First element  - max of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"xrget visits:202001 1000 visits:202003<br />" .
			"xrmax visits:202001      visits:202003</pre>",

			"<pre>select last(val) from table where key >= [key] and key < [range_end] limit 65'536</pre>" .
			"(Note MySQL does not support last(), but PostgreSQL does)"
	),

	new Cmd(
			"XNAVG",

			"XNAVG key prefix",

			"Accumulate using <b>*fake*</b> AVG up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"<br />" .
			"Since average is not a monoid, XNAVG / XRAVG will return correct result, only when all information is accumulated at once.<br />" .
			"if it go to next 'pages', result will be incorrect, but still kind of useful.",

			"array",
			"First element  - max of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.5",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"xnget visits:202001 1000 visits:202001<br />" .
			"xnavg visits:202001      visits:202001<br />" .
			"<br />" .
			"xnget visits:2020   1000 visits:2020<br />" .
			"xnavg visits:2020        visits:2020</pre>",

			"<pre>select avg(val) from table where key >= [key] and key like '[key]%' limit 65'536</pre>" .
			"(Note MySQL also will do fake average, if fhis statement is used)"
	),

	new Cmd(
			"XRAVG",

			"XRAVG key prefix",

			"Accumulate using <b>*fake*</b> AVG up to 65'536 key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.<br />" .
			"<br />" .
			"Since average is not a monoid, XNAVG / XRAVG will return correct result, only when all information is accumulated at once.<br />" .
			"if it go to next 'pages', result will be incorrect, but still kind of useful.",

			"array",
			"First element  - max of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"accumulators_x",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"xrget visits:202001 1000 visits:202003<br />" .
			"xrmax visits:202001      visits:202003</pre>",

			"<pre>select avg(val) from table where key >= [key] and key < [range_end] limit 65'536</pre>" .
			"(Note MySQL also will do fake average, if fhis statement is used)"
	),
);


<?php
return array(
	new Cmd(
			"XNCOUNT",

			"XNCOUNT / COUNT key number prefix",

			"Accumulate using COUNT <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Accumulate up to 10'000 elements.<br />" .
			"<br />" .
			"<b>This command is similar to following MySQL statement:</b><br />" .
			"<pre>select count(*) from table where key >= [key] and key like '[key]%' limit [number]</pre>",


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
			"xncount dom:google: 1000 dom:google:</pre>"

	),

	new Cmd(
			"XRCOUNT",

			"XRCOUNT key number range_end",

			"Accumulate using COUNT <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.<br />" .
			"Accumulate up to 10'000 elements.<br />" .
			"<br />" .
			"<b>This command is similar to following MySQL statement:</b><br />" .
			"<pre>select count(*) from table where key >= [key] and key < [range_end] limit [number]</pre>",

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
			"xrcount dom:google: 1000 dom:google:blogger.com</pre>"
	),

	new Cmd(
			"XNSUM",

			"XNSUM / SUM key number prefix",

			"Accumulate using SUM <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Accumulate up to 10'000 elements.<br />" .
			"<br />" .
			"<b>This command is similar to following MySQL statement:</b><br />" .
			"<pre>select sum(val) from table where key >= [key] and key like '[key]%' limit [number]</pre>",

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
			"xnsum visits:202001 1000 visits:202001<br />" .
			"<br />" .
			"xnget visits:2020   1000 visits:2020<br />" .
			"xnsum visits:2020   1000 visits:2020</pre>"
	),

	new Cmd(
			"XRSUM",

			"XRSUM key number range_end",

			"Accumulate using SUM <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.<br />" .
			"Accumulate up to 10'000 elements.<br />" .
			"<br />" .
			"<b>This command is similar to following MySQL statement:</b><br />" .
			"<pre>select sum(val) from table where key >= [key] and key < [range_end] limit [number]</pre>",

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
			"xrsum visits:202001 1000 visits:202003</pre>"
	),

	new Cmd(
			"XNMIN",

			"XNMIN key number prefix",

			"Accumulate using MIN <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Accumulate up to 10'000 elements.<br />" .
			"<br />" .
			"<b>This command is similar to following MySQL statement:</b><br />" .
			"<pre>select min(val) from table where key >= [key] and key like '[key]%' limit [number]</pre>",

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
			"xnmin visits:202001 1000 visits:202001<br />" .
			"<br />" .
			"xnget visits:2020   1000 visits:2020<br />" .
			"xnmin visits:2020   1000 visits:2020</pre>"
	),

	new Cmd(
			"XRMIN",

			"XRMIN key number range_end",

			"Accumulate using MIN <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.<br />" .
			"Accumulate up to 10'000 elements.<br />" .
			"<br />" .
			"<b>This command is similar to following MySQL statement:</b><br />" .
			"<pre>select min(val) from table where key >= [key] and key < [range_end] limit [number]</pre>",

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
			"xrmin visits:202001 1000 visits:202003</pre>"
	),

	new Cmd(
			"XNMAX",

			"XNMAX key number prefix",

			"Accumulate using MAX <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Accumulate up to 10'000 elements.<br />" .
			"<br />" .
			"<b>This command is similar to following MySQL statement:</b><br />" .
			"<pre>select max(val) from table where key >= [key] and key like '[key]%' limit [number]</pre>",

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
			"xnmax visits:202001 1000 visits:202001<br />" .
			"<br />" .
			"xnget visits:2020   1000 visits:2020<br />" .
			"xnmax visits:2020   1000 visits:2020</pre>"
	),

	new Cmd(
			"XRMAX",

			"XRMAX key number prefix",

			"Accumulate using MAX <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.<br />" .
			"Accumulate up to 10'000 elements.<br />" .
			"<br />" .
			"<b>This command is similar to following MySQL statement:</b><br />" .
			"<pre>select max(val) from table where key >= [key] and key < [range_end] limit [number]</pre>",

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
			"xrmax visits:202001 1000 visits:202003</pre>"
	),

	new Cmd(
			"XNAVG",

			"XNAVG key number prefix",

			"Accumulate using <b>*fake*</b> AVG <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Accumulate up to 10'000 elements.<br />" .
			"<br />" .
			"Since average is not a monoid, XNAVG / XRAVG will return correct result, only when all information is accumulated at once.<br />" .
			"if it go to next 'pages', result will be incorrect, but still kind of useful.<br />" .
			"<br />" .
			"<b>This command is similar to following MySQL statement:</b><br />" .
			"<pre>select avg(val) from table where key >= [key] and key like '[key]%' limit [number]</pre>" .
			"(Note MySQL still will do fake avegare if it uses fhis statement)",

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
			"xnavg visits:202001 1000 visits:202001<br />" .
			"<br />" .
			"xnget visits:2020   1000 visits:2020<br />" .
			"xnavg visits:2020   1000 visits:2020</pre>"
	),

	new Cmd(
			"XRAVG",

			"XRAVG key number prefix",

			"Accumulate using <b>*fake*</b> AVG <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.<br />" .
			"Accumulate up to 10'000 elements.<br />" .
			"<br />" .
			"Since average is not a monoid, XNAVG / XRAVG will return correct result, only when all information is accumulated at once.<br />" .
			"if it go to next 'pages', result will be incorrect, but still kind of useful.<br />" .
			"<br />" .
			"<b>This command is similar to following MySQL statement:</b><br />" .
			"<pre>select avg(val) from table where key >= [key] and key < [range_end] limit [number]</pre>" .
			"Second element - last key, if there is second page.",

			"array",
			"First element  - max of valid elements.<br />",

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
			"xrmax visits:202001 1000 visits:202003</pre>".
			"(Note MySQL still will do fake avegare if it uses fhis statement)",
	),
);

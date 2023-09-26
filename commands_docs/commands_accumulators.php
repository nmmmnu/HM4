<?php
return array(
	new Cmd(
			"COUNTX / COUNT",

			"COUNTX key number prefix",

			"Accumulate using COUNT <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Accumulate up to 10'000 elements.",


			"array",
			"First element  - count of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.4",
			"READ",
			false,
			false,

			"accumulators",

			"<pre>set dom:google:google.com  some_data<br />" .
			"set dom:google:youtube.com some_data<br />" .
			"set dom:google:gmail.com   some_data<br />" .
			"set dom:google:blogger.com some_data<br />" .
			"set dom:google:abc.xyz     some_data<br />" .
			"<br />" .
			"getx   dom:google:  1000 dom:google:<br />" .
			"countx dom:google:       dom:google:</pre>"

	),

	new Cmd(
			"COUNTXR",

			"COUNTXR key number range_end",

			"Accumulate using COUNT <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.<br />" .
			"Accumulate up to 10'000 elements.",

			"array",
			"First element  - count of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"accumulators",

			"<pre>set dom:google:google.com  some_data<br />" .
			"set dom:google:youtube.com some_data<br />" .
			"set dom:google:gmail.com   some_data<br />" .
			"set dom:google:blogger.com some_data<br />" .
			"set dom:google:abc.xyz     some_data<br />" .
			"<br />" .
			"getxr   dom:google:  1000 dom:google:blogger.com<br />" .
			"countxr dom:google:       dom:google:blogger.com</pre>"
	),

	new Cmd(
			"SUMX / SUM",

			"SUMX key number prefix",

			"Accumulate using SUM <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Accumulate up to 10'000 elements.",

			"array",
			"First element  - sum of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.4",
			"READ",
			false,
			false,

			"accumulators",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"getx visits:202001  1000 visits:202001<br />" .
			"sumx visits:202001       visits:202001<br />" .
			"<br />" .
			"getx visits:2020    1000 visits:2020<br />" .
			"sumx visits:2020         visits:2020</pre>"
	),

	new Cmd(
			"SUMXR",

			"SUMXR key number range_end",

			"Accumulate using SUM <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.<br />" .
			"Accumulate up to 10'000 elements.",

			"array",
			"First element  - sum of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"accumulators",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"getxr visits:202001  1000 visits:202003<br />" .
			"sumxr visits:202001       visits:202003</pre>"
	),

	new Cmd(
			"MINX / MIN",

			"MINX key number prefix",

			"Accumulate using MIN <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Accumulate up to 10'000 elements.",

			"array",
			"First element  - min of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.5",
			"READ",
			false,
			false,

			"accumulators",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"getx visits:202001  1000 visits:202001<br />" .
			"minx visits:202001       visits:202001<br />" .
			"<br />" .
			"getx visits:2020    1000 visits:2020<br />" .
			"minx visits:2020         visits:2020</pre>"
	),

	new Cmd(
			"MINXR",

			"MINXR key number range_end",

			"Accumulate using MIN <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.<br />" .
			"Accumulate up to 10'000 elements.",

			"array",
			"First element  - min of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"accumulators",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"getxr visits:202001  1000 visits:202003<br />" .
			"minxr visits:202001       visits:202003</pre>"
	),

	new Cmd(
			"MAXX / MAX",

			"MAXX key number prefix",

			"Accumulate using MAX <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Accumulate up to 10'000 elements.",

			"array",
			"First element  - max of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.5",
			"READ",
			false,
			false,

			"accumulators",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"getx visits:202001  1000 visits:202001<br />" .
			"maxx visits:202001       visits:202001<br />" .
			"<br />" .
			"getx visits:2020    1000 visits:2020<br />" .
			"maxx visits:2020         visits:2020</pre>"
	),

	new Cmd(
			"MAXXR",

			"MAXXR key number prefix",

			"Accumulate using MAX <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are less than or equal the <i>range_end</i>.<br />" .
			"Accumulate up to 10'000 elements.",

			"array",
			"First element  - max of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.3.7.1",
			"READ",
			false,
			false,

			"accumulators",

			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"<br />" .
			"getxr visits:202001  1000 visits:202003<br />" .
			"maxxr visits:202001       visits:202003</pre>"
	),
);

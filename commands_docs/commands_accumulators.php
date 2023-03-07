<?php
return array(
	new Cmd(
			"COUNT",

			"COUNT key number prefix",

			"Accumulate using COUNT <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Accumulate up to 10'000 elements.",

			"array",
			"First element  - count of valid elements.<br />" .
			"Second element - last key, if there is second page.<br />" .
			"<br />" .
			"<u>Example:</u><br />" .
			"<br />" .
			"<pre>set dom:google:google.com  some_data<br />" .
			"set dom:google:youtube.com some_data<br />" .
			"set dom:google:gmail.com   some_data<br />" .
			"set dom:google:blogger.com some_data<br />" .
			"set dom:google:abc.xyz     some_data<br />" .
			"count dom:google: 10000 dom:google:</pre>",

			"1.2.4",
			"READ",
			false,
			false,

			"accumulators"
	),

	new Cmd(
			"SUM",

			"SUM key number prefix",

			"Accumulate using SUM <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"See COUNT for details.<br />" .
			"<br />" .
			"<u>Example:</u><br />" .
			"<br />" .
			"<pre>set visits:20200101 123<br />" .
			"set visits:20200102 263<br />" .
			"set visits:20200103 173<br />" .
			"set visits:20200104 420<br />" .
			"set visits:20200105 345<br />" .
			"sum visits:202001 10000 visits:202001<br />" .
			"sum visits:2020   10000 visits:2020</pre>",

			"array",
			"First element  - sum of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.4",
			"READ",
			false,
			false,

			"accumulators"
	),

	new Cmd(
			"MIN",

			"MIN key number prefix",

			"Accumulate using MIN <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"See COUNT for details.",

			"array",
			"First element  - min of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.5",
			"READ",
			false,
			false,

			"accumulators"
	),

	new Cmd(
			"MAX",

			"MAX key number prefix",

			"Accumulate using MAX <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"See COUNT for details.",

			"array",
			"First element  - max of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.5",
			"READ",
			false,
			false,

			"accumulators"
	),
);

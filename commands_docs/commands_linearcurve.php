<?php
return array(
	new Cmd(
			"MC1GET",

			"MC1GET key subkey",

			"Get value of the <i>subkey</i> stored in <i>key</i>.",
			"string",
			"value of the subkey",
			"1.3.7.7",
			"2 * READ",
			false,
			false,

			"linearcurve"
	),

	new Cmd(
			"MC1MGET",

			"MC1MGET key subkey [subkey]...",

			"Get values of the <i>subkey</i> stored in <i>key</i>.",
			"array",
			"values of the items",
			"1.3.7.7",
			"[number of items] * 2 * READ",
			false,
			false,

			"linearcurve"
	),

	new Cmd(
			"MC1EXISTS",

			"MC1EXISTS key subkey",

			"Check if <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"0 if the item do not exists.<br />" .
			"1 if the item exists.",
			"1.3.7.7",
			"2 * READ",
			false,
			false,

			"linearcurve"
	),

	new Cmd(
			"MC1SCORE",

			"MC1SCORE key subkey",

			"Get score of the <i>subkey</i> stored in <i>key</i>.",
			"array",
			"x",
			"1.3.7.7",
			"READ",
			false,
			false,

			"linearcurve"
	),

	new Cmd(
			"MC1ADD",

			"MC1ADD key subKey x value [subKey x value]...",

			"Set <i>subkey</i> with 1D coordinate <i>x</i> and <i>value</i> in <i>key</i>.<br />" .
			"Uses <b>uint32_t</b> for coordinates.",
			"OK",
			"OK",
			"1.3.7.7",
			"[number of items] * (READ + 3 * WRITE), TX",
			false,
			true,

			"linearcurve"
	),

	new Cmd(
			"MC1REM",

			"MC1REM key subkey [subkey]...",

			"Removes <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"Always return 1",
			"1.3.7.7",
			"[number of items] * (READ + 2 * WRITE), TX",
			false,
			true,

			"linearcurve"
	),

	new Cmd(
			"MC1POINT",

			"MC1POINT key x number [start]",

			"Gets all subkeys stored in 'point' with 1D coordinate <i>x</i>.<br />" .
			"Uses <b>uint32_t</b> for coordinates.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"Return up to <i>number</i> of pairs.<br />" .
			"Returns up to ~32'000 elements.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.7.7",
			"READ",
			false,
			false,

			"linearcurve",

			"<pre>" .
			"<i>Suppose we have a company with two offices in LA and NYC (LA=1, NYC=2).</i><br />" .
			"<br />" .
			"mc1add users 0 1 john          <i>adds user_id 0, john,   LA</i><br />" .
			"mc1add users 1 1 peter         <i>adds user_id 1, peter,  LA</i><br />" .
			"mc1add users 2 1 jill          <i>adds user_id 2, jill,   LA</i><br />" .

			"mc1add users 3 2 robert        <i>adds user_id 3, robert, NYC</i><br />" .
			"mc1add users 4 2 pola          <i>adds user_id 4, pola,   NYC</i><br />" .
			"mc1add users 5 2 pepe          <i>adds user_id 5, pepe,   NYC</i><br />" .

			"mc1point users 2 10000         <i>gets all from LA        (robert, pola, pepe)</i><br />" .
			"<br />" .
			"xnget users~ 1000 users~</pre>",

			"<pre>select key, val from table where x = [x] limit [number]</pre>"
	),

	new Cmd(
			"MC1RANGE",

			"MC1RANGE key x_min x_max_min y_max number [start]",

			"Gets all subkeys stored in 'rectangle' with 1D coordinate <i>x_min</i> and <i>y_min</i> to <i>x_max</i> and <i>y_max</i>.<br />" .
			"Uses <b>uint32_t</b> for coordinates.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"Return up to <i>number</i> of pairs.<br />" .
			"Returns up to ~32'000 elements.<br />" .
			"Note this command is useful only on very large datasets.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.7.7",
			"READ",
			false,
			false,

			"linearcurve",

			"<pre>" .
			"<i>Suppose we have a company with two offices in LA and NYC (LA=1, NYC=2).</i><br />" .
			"<br />" .
			"mc1add users 0 1 john          <i>adds user_id 0, john,   LA</i><br />" .
			"mc1add users 1 1 peter         <i>adds user_id 1, peter,  LA</i><br />" .
			"mc1add users 2 1 jill          <i>adds user_id 2, jill,   LA</i><br />" .

			"mc1add users 3 2 robert        <i>adds user_id 3, robert, NYC</i><br />" .
			"mc1add users 4 2 pola          <i>adds user_id 4, pola,   NYC</i><br />" .
			"mc1add users 5 2 pepe          <i>adds user_id 5, pepe,   NYC</i><br />" .

			"mc1point users 2   10000       <i>gets all from LA        (robert, pola, pepe)</i><br />" .
			"mc1range users 2 2 10000       <i>gets all from LA        (robert, pola, pepe) - same as previous, same speed.</i><br />" .
			"<br />" .
			"xnget users~ 1000 users~</pre>",

			"<pre>select key, val from table where x >= [x_min] and x <= [x_max] limit [number]</pre>"
	),



);

<?php
return array(
	new Cmd(
			"MC2GET",

			"MC2GET key subkey",

			"Get value of the <i>subkey</i> stored in <i>key</i>.",
			"string",
			"value of the subkey",
			"1.3.7.7",
			"2 * READ",
			false,
			false,

			"mortoncurve"
	),

	new Cmd(
			"MC2MGET",

			"MC2MGET key subkey [subkey]...",

			"Get values of the <i>subkey</i> stored in <i>key</i>.",
			"array",
			"values of the items",
			"1.3.7.7",
			"[number of items] * 2 * READ",
			false,
			false,

			"mortoncurve"
	),

	new Cmd(
			"MC2EXISTS",

			"MC2EXISTS key subkey",

			"Check if <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"0 if the item do not exists.<br />" .
			"1 if the item exists.",
			"1.3.7.7",
			"2 * READ",
			false,
			false,

			"mortoncurve"
	),

	new Cmd(
			"MC2SCORE",

			"MC2SCORE key subkey",

			"Get score of the <i>subkey</i> stored in <i>key</i>.",
			"array",
			"x and y",
			"1.3.7.7",
			"READ",
			false,
			false,

			"mortoncurve"
	),

	new Cmd(
			"MC2ADD",

			"MC2ADD key subKey x y value [subKey x y value]...",

			"Set <i>subkey</i> with 2D coordinates <i>x</i> and <i>y</i> and <i>value</i> in <i>key</i>.<br />" .
			"Uses <b>uint32_t</b> for coordinates.",
			"OK",
			"OK",
			"1.3.7.7",
			"[number of items] * (READ + 3 * WRITE), TX",
			false,
			true,

			"mortoncurve"
	),

	new Cmd(
			"MC2REM",

			"MC2REM key subkey [subkey]...",

			"Removes <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"Always return 1",
			"1.3.7.7",
			"[number of items] * (READ + 2 * WRITE), TX",
			false,
			true,

			"mortoncurve"
	),

	new Cmd(
			"MC2POINT",

			"MC2POINT key x y number [start]",

			"Gets all subkeys stored in 'point' with 2D coordinates <i>x</i> and <i>y</i>.<br />" .
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

			"mortoncurve",

			"<pre>" .
			"<i>Suppose we have a company with two offices in LA and NYC (LA=1, NYC=2).</i><br />" .
			"<i>Each office have two kind of employees - sales and support (sales=1 and support=2).</i><br />" .
			"<br />" .
			"mc2add users 0 1 1 john          <i>adds user_id 0, john,   sales,   LA</i><br />" .
			"mc2add users 1 1 1 peter         <i>adds user_id 1, peter,  sales,   LA</i><br />" .
			"mc2add users 2 2 1 jill          <i>adds user_id 2, jill,   support, LA</i><br />" .

			"mc2add users 3 1 2 robert        <i>adds user_id 3, robert, sales,   NYC</i><br />" .
			"mc2add users 4 2 2 pola          <i>adds user_id 4, pola,   support, NYC</i><br />" .
			"mc2add users 5 2 2 pepe          <i>adds user_id 5, pepe,   support, NYC</i><br />" .

			"mc2point users 2 2 10000         <i>gets all support from LA               (pola, pepe)</i><br />" .
			"<br />" .
			"xnget users~ 1000 users~</pre>",

			"<pre>select key, val from table where x = [x] and y = [y] limit [number]</pre>"
	),

	new Cmd(
			"MC2RANGE",

			"MC2RANGE key x_min x_max y_min y_max number [start]",

			"Gets all subkeys stored in 'rectangle' with 2D coordinates <i>x_min</i> and <i>y_min</i> to <i>x_max</i> and <i>y_max</i>.<br />" .
			"Uses <b>uint32_t</b> for coordinates.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"Return up to <i>number</i> of pairs.<br />" .
			"Returns up to ~32'000 elements.<br />" .
			"Note this command is useful only on very large datasets.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.7.7",
			"N * READ, were N &isin; { 1 .. 9 }",
			false,
			false,

			"mortoncurve",

			"<pre>" .
			"<i>Suppose we have a company with two offices in LA and NYC (LA=1, NYC=2).</i><br />" .
			"<i>Each office have two kind of employees - sales and support (sales=1 and support=2).</i><br />" .
			"<br />" .
			"mc2add users 0 1 1 john          <i>adds user_id 0, john,   sales,   LA</i><br />" .
			"mc2add users 1 1 1 peter         <i>adds user_id 1, peter,  sales,   LA</i><br />" .
			"mc2add users 2 2 1 jill          <i>adds user_id 2, jill,   support, LA</i><br />" .

			"mc2add users 3 1 2 robert        <i>adds user_id 3, robert, sales,   NYC</i><br />" .
			"mc2add users 4 2 2 pola          <i>adds user_id 4, pola,   support, NYC</i><br />" .
			"mc2add users 5 2 2 pepe          <i>adds user_id 5, pepe,   support, NYC</i><br />" .

			"mc2point users 2 2 10000         <i>gets all support from LA               (pola, pepe)</i><br />" .
			"mc2range users 2 2 2 2 10000     <i>gets all support from LA               (pola, pepe) - same as previous, but slower.</i><br />" .

			"mc2range users 2 2 1 2 10000     <i>gets support from LA and NYC           (jill, pola, pepe)</i><br />" .
			"mc2range users 1 2 2 2 10000     <i>gets sales and support from LA         (robert, pola, pepe)</i><br />" .
			"mc2range users 1 2 1 2 10000     <i>gets sales and support from LA and NYC (all)</i><br />" .
			"<br />" .
			"xnget users~ 1000 users~</pre>",

			"<pre>select key, val from table where x >= [x_min] and x <= [x_max] and y >= [y_min] and y <= [y_max] limit [number]</pre>"
	),

	new Cmd(
			"MC2RANGENAIVE",

			"MC2RANGENAIVE key x_min x_max y_min y_max number [start]",

			"Naive, non performant version of MC2XNGET. Made for small datasets and for testing.<br />" .
			"See MC2RANGE for more information.<br />" .
			"Uses <b>uint32_t</b> for coordinates.<br />" .
			"Technically the data lookup complexity is single READ, but on large datasets, it might need to read complete dataset.",
			"array",
			"results",
			"1.3.7.7",
			"READ",
			false,
			false,

			"mortoncurve"
	),

	new Cmd(
			"MC2ENCODE",

			"MC2ENCODE x y",

			"Encode <i>x</i> and <i>y</i> to morton code hex string.<br />" .
			"Uses <b>uint32_t</b> for coordinates.",
			"string",
			"morton code hex string",
			"1.3.7.7",
			"READ",
			false,
			null,

			"mortoncurve"
	),

	new Cmd(
			"MC2DECODE",

			"MC2DECODE hex",

			"Decode <i>morton code hex string</i> to x, y.",
			"array",
			"x and y",
			"1.3.7.7",
			"READ",
			false,
			null,

			"mortoncurve"
	),

);

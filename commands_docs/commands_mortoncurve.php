<?php
return array(
	new Cmd(
			"MC2GET",

			"MC2GET key x y",

			"Get value of the item stored in <i>key</i> with 2D coordinates <i>x</i> and <i>y</i>.<br />" .
			"Uses <b>uint32_t</b> for coordinates.",
			"string",
			"value of the item",
			"1.3.7.7",
			"READ",
			false,
			false,

			"mortoncurve"
	),

	new Cmd(
			"MC2MGET",

			"MC2MGET key x y [x1 y1]...",

			"Get values of the items stored in <i>key</i> with 2D coordinates <i>x</i> and <i>y</i>, <i>x1</i> and <i>y1</i>...<br />" .
			"Uses <b>uint32_t</b> for coordinates.",
			"array",
			"values of the items",
			"1.3.7.7",
			"[number of items] * READ",
			false,
			false,

			"mortoncurve"
	),

	new Cmd(
			"MC2EXISTS",

			"MC2EXISTS key x y",

			"Check if item stored in <i>key</i> with 2D coordinates <i>x</i> and <i>y</i> exists.<br />" .
			"Uses <b>uint32_t</b> for coordinates.",
			"bool",
			"0 if the item do not exists.<br />" .
			"1 if the item exists.",
			"1.3.7.7",
			"READ",
			false,
			false,

			"mortoncurve"
	),

	new Cmd(
			"MC2SET",

			"MC2GET key x y value [x1 y1 value1]...",

			"Set <i>value</i> of the item stored in <i>key</i> with 2D coordinates <i>x</i> and <i>y</i>.<br />" .
			"Can set multiple values at once.<br />" .
			"Uses <b>uint32_t</b> for coordinates.",
			"OK",
			"OK",
			"1.3.7.7",
			"[number of items] * WRITE",
			false,
			true,

			"mortoncurve"
	),

	new Cmd(
			"MC2DEL",

			"MC2DEL key x y [x1 y1]...",

			"Removes items stored in <i>key</i> with 2D coordinates <i>x</i> and <i>y</i>, <i>x1</i> and <i>y1</i>...<br />" .
			"Uses <b>uint32_t</b> for coordinates.",
			"bool",
			"Always return 1",
			"1.3.7.7",
			"[number of items] * WRITE",
			false,
			true,

			"mortoncurve"
	),

	new Cmd(
			"MC2XNGET",

			"MC2XNGET key x_min x_max y_min y_max number [start]",

			"Gets a 'rectangle' with 2D coordinates <i>x_min</i> and <i>y_min</i> to <i>x_max</i> and <i>y_max</i>.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET.<br />" .
			"Return ONLY items from valid pairs.<br />" .
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
			"<i>Suppose we have a company with two offices in LA and NYC.</i><br />" .
			"<i>Each office have just two people - sales representative and support representative.</i><br />" .
			"<i>Lets define coordinates x=1/2 means sales / support representatives and  y=1/2 means LA / NYC offices</i><br />" .
			"<i>Note the combinations are distinct. There can be only one item on each coordinate.</i><br />" .
			"<br />" .
			"mc2set users 1 1 john@example.com,peter@example.com   <i>adds sales   from LA, two people, so we fit them in single item</i><br />" .
			"mc2set users 2 1 jill@example.com                     <i>adds support from LA</i><br />" .
			"mc2set users 1 2 robert@example.com                   <i>adds sales   from NYC</i><br />" .
			"mc2set users 2 2 pola@example.com                     <i>adds support from NYC</i><br />" .
			"MC2GET users 1 1                                      <i>gets sales from LA</i><br />" .
			"MC2XNGET users 1 1 1 1 10000                          <i>gets sales from LA - same as previous</i><br />" .
			"MC2XNGET users 1 1 1 2 10000                          <i>gets sales from LA and NYC</i><br />" .
			"MC2XNGET users 1 2 1 1 10000                          <i>gets sales and support from LA</i><br />" .
			"MC2XNGET users 1 2 1 2 10000                          <i>gets sales and support from LA and NYC</i><br />" .
			"<br />" .
			"xnget users~ 1000 users~</pre>",

			"<pre>select key, val from table where x between [x_min] and [x_max] and y between [y_min] and [y_max] limit [number]</pre>"
	),

	new Cmd(
			"MC2XNGETNAIVE",

			"MC2XNGETNAIVE key x_min x_max y_min y_max number [start]",

			"Naive, non performant version of MC2XNGET. Made for small datasets and for testing.<br />" .
			"See MC2XNGET for more information.<br />" .
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

);

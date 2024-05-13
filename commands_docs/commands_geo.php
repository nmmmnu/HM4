<?php
return array(
	new Cmd(
			"GEOGET",

			"GEOGET / GEOSCORE key name",

			"Retrieve the specified geospatial item (<i>name</i>) from the specified <i>key</i>.",
			"string",
			"[latitude],[longitude],[geohash]",
			"1.3.4.2",
			"READ",
			false,
			false,

			"geo",

			"<pre>geoadd places 42.69174997126510 23.32100561258516 Boho<br />" .
			"geoadd places 42.68342508736217 23.31633975360111 Chevermeto<br />" .
			"geoget places Boho<br />" .
			"geoget places Chevermeto</pre>"
	),

	new Cmd(
			"GEOMGET",

			"GEOMGET key name [name]...",

			"Retrieve the specified geospatial items (<i>name</i>) from the specified <i>key</i>.",
			"array",
			"array",
			"1.3.4.2",
			"[number of keys] * READ",
			false,
			false,

			"geo",

			"<pre>geoadd  places 42.69174997126510 23.32100561258516 Boho<br />" .
			"geoadd  places 42.68342508736217 23.31633975360111 Chevermeto<br />" .
			"geomget places Boho Chevermeto NonExistent</pre>"
	),

	new Cmd(
			"GEOADD",

			"GEOADD key latitude longitude name [latitude longitude name]...",

			"Adds the specified geospatial items (<i>latitude</i>, <i>longitude</i>, <i>name</i>) to the specified <i>key</i>.<br />" .
			"Command is not Redis compatible, because <i>latitude</i> and <i>longitude</i> are not in the same order as in Redis.",
			"OK",
			"OK",
			"1.3.4.2",
			"[number of keys] * (READ + 3 * WRITE), TX",
			false,
			true,

			"geo",

			"<pre>geoadd places 42.69174997126510 23.32100561258516 Boho<br />" .
			"geoadd places 42.68342508736217 23.31633975360111 Chevermeto<br />" .
			"xnget  places 1000 places</pre>"
	),

	new Cmd(
			"GEOREM",

			"GEOREM / GEOREMOVE / GEODEL key name [name]...",

			"Removes the specified geospatial items (<i>name</i>) from the specified <i>key</i>.",
			"OK",
			"OK",
			"1.3.4.2",
			"[number of keys] * (READ + 2 * WRITE), TX",
			false,
			true,

			"geo",

			"<pre>geoadd places 42.69174997126510 23.32100561258516 Boho<br />" .
			"geoadd places 42.68342508736217 23.31633975360111 Chevermeto<br />" .
			"xnget  places 1000 places<br />" .
			"georem places Chevermeto<br />" .
			"xnget  places 1000 places</pre>"
	),

	new Cmd(
			"GEORADIUS",

			"GEORADIUS / GEORADIUS_RO key latitude longitude radius",

			"Retrieve all geospatial items from the specified <i>key</i> that are within <i>radius</i> <b>meters</b> from specific <i>latitude</i> <i>longitude</i> position.<br />" .
			"Unlike Redis, this works only in meters.<br />" .
			"Similar to Redis this works only on planet Earth :)",
			"array",
			"array",
			"1.3.4.2",
			"N * READ, were N &isin; { 9, 6, 4, 2, 1 }",
			false,
			false,

			"geo",

			"<pre>geoadd    places 42.69174997126510 23.32100561258516 Boho<br />" .
			"geoadd    places 42.68342508736217 23.31633975360111 Chevermeto<br />" .
			"georadius places 42.69209558730095 23.32479448130661 450</pre>"
	),

	new Cmd(
			"GEODIST",

			"GEODIST key name1 name2",

			"Return the distance between two members (<i>name1</i> and <i>name2</i>) from the <i>key</i><br />" .
			"Unlike Redis, this works only in meters.<br />" .
			"Similar to Redis this works only on planet Earth :)",
			"array",
			"array",
			"1.3.4.2",
			"2 * READ",
			false,
			false,

			"geo",

			"<pre>geoadd  places 42.69174997126510 23.32100561258516 Boho<br />" .
			"geoadd  places 42.68342508736217 23.31633975360111 Chevermeto<br />" .
			"geodist places Boho Chevermeto</pre>"
	),

	new Cmd(
			"GEOENCODE",

			"GEOENCODE latitude longitude",

			"Encode <i>latitude</i> and <i>longitude</i> into geohash",
			"string",
			"[latitude],[longitude],[geohash]",
			"1.3.4.2",
			"N/A",
			false,
			null,

			"geo",

			"<pre>geoencode 42.69174997126510 23.32100561258516</pre>"
	),

	new Cmd(
			"GEODECODE",

			"GEODECODE geohash",

			"Encode <i>geohash</i> into latitude and longitude.<br />" .
			"Note if you encode coordinates into geohash and then decode the hash, result may be slightly different coordinates",
			"string",
			"[latitude],[longitude],[geohash]",
			"1.3.4.2",
			"N/A",
			false,
			null,

			"geo",

			"<pre>geodecode sx8dfevc6z40</pre>"
	),

);

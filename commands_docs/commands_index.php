<?php
return array(
	new Cmd(
			"IX*GET",

			"IX1GET / IX2GET / IX3GET key subkey",

			"Get value of the <i>subkey</i> stored in <i>key</i>.",
			"string",
			"value of the subkey",
			"1.3.8",
			"2 * READ",
			false,
			false,

			"index"
	),

	new Cmd(
			"IX*MGET",

			"IX1MGET / IX2MGET / IX3MGET key subkey [subkey]...",

			"Get values of the <i>subkey</i> stored in <i>key</i>.",
			"array",
			"values of the items",
			"1.3.8",
			"[number of items] * 2 * READ",
			false,
			false,

			"index"
	),

	new Cmd(
			"IX*EXISTS",

			"IX1EXISTS / IX2EXISTS / IX3EXISTS key subkey",

			"Check if <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"0 if the item do not exists.<br />" .
			"1 if the item exists.",
			"1.3.8",
			"2 * READ",
			false,
			false,

			"index"
	),

	new Cmd(
			"IX*GETINDEXES",

			"IX1GETINDEXES / IX2GETINDEXES / IX3GETINDEXES key subkey",

			"Get index values (score) of the <i>subkey</i> stored in <i>key</i>.",
			"array",
			"index values",
			"1.3.8",
			"READ",
			false,
			false,

			"index"
	),

	new Cmd(
			"IX1ADD",

			"IX1ADD key subKey index_value1 value [subKey index_value1 value]...",

			"Set <i>subkey</i> with index values <i>index_value1</i> and <i>value</i> in <i>key</i>.",
			"OK",
			"OK",
			"1.3.8",
			"[number of items] * (READ + 3 * WRITE), TX",
			false,
			true,

			"index"
	),

	new Cmd(
			"IX2ADD",

			"IX2ADD key subKey index_value1 index_value2 value [subKey index_value1 index_value2 value]...",

			"Set <i>subkey</i> with index values <i>index_value1, index_value2</i> and <i>value</i> in <i>key</i>.",
			"OK",
			"OK",
			"1.3.8",
			"[number of items] * (READ + 5 * WRITE), TX",
			false,
			true,

			"index"
	),

	new Cmd(
			"IX3ADD",

			"IX3ADD key subKey index_value1 index_value2 index_value3 value [subKey index_value1 index_value2 index_value3 value]...",

			"Set <i>subkey</i> with index values <i>index_value1, index_value2, index_value3</i> and <i>value</i> in <i>key</i>.",
			"OK",
			"OK",
			"1.3.8",
			"[number of items] * (READ + 13 * WRITE), TX",
			false,
			true,

			"index"
	),

	new Cmd(
			"IX*REM",

			"IX1REM / IX2REM / IX3REM key subkey [subkey]...",

			"Removes <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"Always return 1",
			"1.3.8",
			"[number of items] * (READ + {2, 3 or 7} * WRITE), TX",
			false,
			true,

			"index"
	),

	new Cmd(
			"IX1RANGE",

			"IX1RANGE key index_name index_value1 number start",

			"Gets all subkeys stored in the <i>index_name</i> with value <i>index_value1</i>.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"<i>index_name</i> is always \"A\".<br />" .
			"Return up to <i>number</i> of pairs.<br />" .
			"Returns up to ~32'000 elements.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.8",
			"READ",
			false,
			false,

			"index",

			"<pre>" .
			"<i>Suppose we have a company with two offices in LA and NYC.</i><br />" .
			"<br />" .
			"ix1add users1 john   LA data_for_john   <br />" .
			"ix1add users1 peter  LA data_for_peter  <br />" .
			"ix1add users1 jill   LA data_for_jill   <br />" .

			"ix1add users1 robert NY data_for_robert <br />" .
			"ix1add users1 pola   NY data_for_pola   <br />" .
			"ix1add users1 pepe   NY data_for_pepe   <br />" .

			"ix1range users1 A '' 1000 ''            <i>get all users.</i><br />" .
			"ix1range users1 A 'LA' 1000 ''          <i>get all users from LA (john, peter, jill).</i><br />" .
			"<br />" .
			"xnget users1~ 1000 users1~</pre>",

			"<pre>" .
			"select key, val from table using index A                                     order by index1 limit [number]<br />" .
			"...or...<br />" .
			"select key, val from table using index A where index_field1 = 'index_value1' order by index1 limit [number]</pre>"
	),

	new Cmd(
			"IX2RANGE",

			"IX2RANGE key index_name index_value1 index_value2 number start",

			"Gets all subkeys stored in the <i>index_name</i> with value <i>index_value1</i> and <i>index_value2</i>.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"<i>index_name</i> is \"AB\" or \"BA\".<br />" .
			"Return up to <i>number</i> of pairs.<br />" .
			"Returns up to ~32'000 elements.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.8",
			"READ",
			false,
			false,

			"index",

			"<pre>" .
			"<i>Suppose we have a company with two offices in LA and NYC. Each employee is either sales or support.</i><br />" .
			"<br />" .
			"ix2add users2 john   LA sales   data_for_john   <br />" .
			"ix2add users2 peter  LA sales   data_for_peter  <br />" .
			"ix2add users2 jill   LA support data_for_jill   <br />" .

			"ix2add users2 robert NY sales   data_for_robert <br />" .
			"ix2add users2 pola   NY support data_for_pola   <br />" .
			"ix2add users2 pepe   NY support data_for_pepe   <br />" .

			"ix2range users2 AB ''      ''      1000 ''      <i>get all users sorted by city,department.</i><br />" .
			"ix2range users2 AB LA      ''      1000 ''      <i>get all users from LA (john, peter, jill).</i><br />" .
			"ix2range users2 AB LA      support 1000 ''      <i>get all users from LA, department support (jill).</i><br />" .

			"ix2range users2 BA ''      ''      1000 ''      <i>get all users sorted by department,city.</i><br />" .
			"ix2range users2 BA support ''      1000 ''      <i>get all users from department support (jill, pepe, pola).</i><br />" .
			"ix2range users2 BA support LA      1000 ''      <i>get all users from department support, from LA (jill).</i><br />" .

			"<br />" .
			"xnget users2~ 1000 users2~</pre>",

			"<pre>" .
			"select key, val from table using index AB                                                                       order by index_field1, index_field2 limit [number]<br />" .
			"...or...<br />" .
			"select key, val from table using index AB where index_field1 = 'index_value1'                                   order by index_field1, index_field2 limit [number]<br />" .
			"...or...<br />" .
			"select key, val from table using index AB where index_field1 = 'index_value1' and index_field2 = 'index_value2' order by index_field1, index_field2 limit [number]</pre>"
	),

	new Cmd(
			"IX3RANGE",

			"IX3RANGE key index_name index_value1 index_value2 index_value3 number start",

			"Gets all subkeys stored in the <i>index_name</i> with value <i>index_value1</i>, <i>index_value2</i> and <i>index_value3</i>.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"<i>index_name</i> is \"ABC\", \"ACB\", \"BAC\", \"BCA\", \"CAB\", \"CBA\".<br />" .
			"Return up to <i>number</i> of pairs.<br />" .
			"Returns up to ~32'000 elements.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.3.8",
			"READ",
			false,
			false,

			"index",

			"<pre>" .
			"<i>Suppose we have a company with two offices in LA and NYC. Each employee is either sales or support. Each employee also can be senior or junior</i><br />" .
			"<br />" .
			"ix3add users3 john   LA sales   senior data_for_john   <br />" .
			"ix3add users3 peter  LA sales   junior data_for_peter  <br />" .
			"ix3add users3 jill   LA support senior data_for_jill   <br />" .

			"ix3add users3 robert NY sales   senior data_for_robert <br />" .
			"ix3add users3 pola   NY support senior data_for_pola   <br />" .
			"ix3add users3 pepe   NY support junior data_for_pepe   <br />" .

			"ix3range users3 ABC ''      ''      ''      1000 ''      <i>get all users sorted by city,department,senior.</i><br />" .
			"ix3range users3 ABC LA      ''      ''      1000 ''      <i>get all users from LA (john, peter, jill).</i><br />" .
			"ix3range users3 ABC LA      support ''      1000 ''      <i>get all users from LA, department support (jill).</i><br />" .
			"ix3range users3 ABC LA      support senior  1000 ''      <i>get all users from LA, department support, senior employees (jill).</i><br />" .

			"ix3range users3 BAC ''      ''      ''      1000 ''      <i>get all users sorted by department,city,senior.</i><br />" .
			"ix3range users3 BAC support ''      ''      1000 ''      <i>get all users from department support (jill, pepe, pola).</i><br />" .
			"ix3range users3 BAC support LA      ''      1000 ''      <i>get all users from department support, from LA (jill).</i><br />" .
			"ix3range users3 BAC support LA      senior  1000 ''      <i>get all users from department support, from LA, who are senior (jill).</i><br />" .

			"ix3range users3 CAB ''      ''      ''      1000 ''      <i>get all users sorted by senior,city,department.</i><br />" .
			"ix3range users3 CAB senior  ''      ''      1000 ''      <i>get all users who are senior (john, jill, robert, pola).</i><br />" .
			"ix3range users3 CAB senior  LA      ''      1000 ''      <i>get all users who are senior, from LA (john, jill).</i><br />" .
			"ix3range users3 CAB senior  LA      support 1000 ''      <i>get all users who are senior, from LA, from department support (jill).</i><br />" .

			"<br />" .
			"xnget users3~ 1000 users3~</pre>",

			"<pre>" .
			"select key, val from table using index ABC                                                                                                         order by index_field1, index_field2, index_field3 limit [number]<br />" .
			"...or...<br />" .
			"select key, val from table using index ABC where index_field1 = 'index_value1' and index_field2 = 'index_value2'                                   order by index_field1, index_field2, index_field3 limit [number]<br />" .
			"...or...<br />" .
			"select key, val from table using index ABC where index_field1 = 'index_value1' and index_field2 = 'index_value2' and index_field3 = 'index_value3' order by index_field1, index_field2, index_field3 limit [number]</pre>"
	),

);

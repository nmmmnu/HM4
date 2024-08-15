<?php
return array(
	new Cmd(
			"IX*GET",

			"IX1GET / IX2GET / IX3GET / IX4GET / IX5GET key subkey",

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

			"IX1MGET / IX2MGET / IX3MGET / IX4MGET / IX5MGET key subkey [subkey]...",

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

			"IX1EXISTS / IX2EXISTS / IX3EXISTS / IX4EXISTS / IX5EXISTS key subkey",

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

			"IX1GETINDEXES / IX2GETINDEXES / IX3GETINDEXES / IX4GETINDEXES / IX5GETINDEXES key subkey",

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
			"[number of items] * (READ + (1 + 2 * 1) * WRITE), TX",
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
			"[number of items] * (READ + (1 + 2 * 2) * WRITE), TX",
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
			"[number of items] * (READ + (1 + 2 * 6) * WRITE), TX",
			false,
			true,

			"index"
	),

	new Cmd(
			"IX4ADD",

			"IX4ADD key subKey index_value1 index_value2 index_value3 index_value4 value [subKey index_value1 index_value2 index_value3 index_value4 value]...",

			"Set <i>subkey</i> with index values <i>index_value1, index_value2, index_value3, index_value4</i> and <i>value</i> in <i>key</i>.",
			"OK",
			"OK",
			"1.3.8",
			"[number of items] * (READ + (1 + 2 * 24) * WRITE), TX",
			false,
			true,

			"index"
	),

	new Cmd(
			"IX5ADD",

			"IX5ADD key subKey index_value1 index_value2 index_value3 index_value4 value [subKey index_value1 index_value2 index_value3 index_value4 index_value5 value]...",

			"Set <i>subkey</i> with index values <i>index_value1, index_value2, index_value3, index_value4, index_value5</i> and <i>value</i> in <i>key</i>.",
			"OK",
			"OK",
			"1.3.8",
			"[number of items] * (READ + (1 + 2 * 120) * WRITE), TX",
			false,
			true,

			"index"
	),

	new Cmd(
			"IX*REM",

			"IX1REM / IX2REM / IX3REM / IX4REM / IX5REM key subkey [subkey]...<br />" .
			"IX1REMOVE / IX2REMOVE / IX3REMOVE / IX4REMOVE / IX5REMOVE key subkey [subkey]...<br />" .
			"IX1DEL / IX2DEL / IX3DEL / IX4DEL / IX5DEL key subkey [subkey]...",

			"Removes <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"Always return 1",
			"1.3.8",
			"[number of items] * (READ + {1+1, 1+2, 1+6, 1+24 or 1+120} * WRITE), TX",
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
"select key, val
from table
using index A
order by
   index_field1
limit [number]

...or...

select key, val
from table
using index A
where
   index_field1 = 'index_value1'
order by
   index_field1
limit [number]</pre>"
	),

	new Cmd(
			"IX2RANGE",

			"IX2RANGE key index_name index_value1 index_value2 number start",

			"Gets all subkeys stored in the <i>index_name</i> with value <i>index_value1</i> and <i>index_value2</i>.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"<i>index_name</i> are the permutations of 2 elements, e.g. \"AB\" or \"BA\".<br />" .
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
"select key, val
from table
using index AB
order by
   index_field1,
   index_field2
limit [number]

...or...

select key, val
from table
using index AB
where
   index_field1 = 'index_value1'
order by
   index_field1,
   index_field2
limit [number]

...or...

select key, val
from table
using index AB
where
   index_field1 = 'index_value1' and
   index_field2 = 'index_value2'
order by
   index_field1,
   index_field2
limit [number]
</pre>"
	),

	new Cmd(
			"IX3RANGE",

			"IX3RANGE key index_name index_value1 index_value2 index_value3 number start",

			"Gets all subkeys stored in the <i>index_name</i> with value <i>index_value1</i>, <i>index_value2</i> and <i>index_value3</i>.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"<i>index_name</i> are the permutations of 3 elements, e.g. \"ABC\", \"ACB\", \"BAC\", \"BCA\", \"CAB\", \"CBA\".<br />" .
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
"select key, val
from table
using index ABC
order by
order by
   index_field1,
   index_field2,
   index_field3
limit [number]

...or...

select key, val
from table
using index ABC
where
   index_field1 = 'index_value1'
order by
   index_field1,
   index_field2,
   index_field3
[number]

...or...

select key, val
from table
using index ABC
where
   index_field1 = 'index_value1' and
   index_field2 = 'index_value2'
order by
   index_field1,
   index_field2,
   index_field3
[number]

...or...

select key, val
from table
using index ABC
where
   index_field1 = 'index_value1' and
   index_field2 = 'index_value2' and
   index_field3 = 'index_value3'
order by
   index_field1,
   index_field2,
   index_field3
limit [number]</pre>"
	),

	new Cmd(
			"IX4RANGE",

			"IX4RANGE key index_name index_value1 index_value2 index_value3 index_value4 number start",

			"Gets all subkeys stored in the <i>index_name</i> with value <i>index_value1</i>, <i>index_value2</i>, <i>index_value3</i> and <i>index_value4</i>.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"<i>index_name</i> are the permutations of 4 elements, e.g. \"ABCD\", \"ABDC\", \"ACBD\", \"ACDB\", \"ADBC\", \"ADCB\" ...<br />" .
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
			"<i>Suppose we are selling laptops</i><br />" .
			"<br />" .
			"ix4add stock4 zen    ASUS 13 16 1 'Asus Zenbook,  Intel i7, 13 inch display, 16 GB RAM, 1 TB SSD' <br />" .

			"ix4add stock4 xps13  DELL 13 16 1 'Dell XPS13 13, Intel i7, 13 inch display, 16 GB RAM, 1 TB SSD' <br />" .

			"ix4add stock4 xps15  DELL 15 16 1 'Dell XPS15 15, Intel i7, 15 inch display, 16 GB RAM, 1 TB SSD' <br />" .
			"ix4add stock4 xps15b DELL 15 32 2 'Dell XPS15 15, Intel i7, 15 inch display, 32 GB RAM, 1 TB SSD' <br />" .
			"ix4add stock4 xps15c DELL 15 16 2 'Dell XPS15 15, Intel i7, 15 inch display, 16 GB RAM, 2 TB SSD' <br />" .
			"ix4add stock4 xps15d DELL 15 32 2 'Dell XPS15 15, Intel i7, 15 inch display, 32 GB RAM, 2 TB SSD' <br />" .

			"ix4add stock4 xps17  DELL 17 32 2 'Dell XPS17 17, Intel i7, 17 inch display, 32 GB RAM, 2 TB SSD' <br />" .

			"ix4range stock4 ABCD ''   '' '' '' 1000 ''      <i>get all laptops sorted by brand, screen size, RAM, disk size.</i><br />" .
			"ix4range stock4 ABCD DELL '' '' '' 1000 ''      <i>get all laptops from DELL.</i><br />" .
			"ix4range stock4 ABCD DELL 15 '' '' 1000 ''      <i>get all laptops from DELL, 15 inch display.</i><br />" .
			"ix4range stock4 ABCD DELL 15 16 '' 1000 ''      <i>get all laptops from DELL, 15 inch display, 16 GB RAM.</i><br />" .
			"ix4range stock4 ABCD DELL 15 16 2  1000 ''      <i>get all laptops from DELL, 15 inch display, 16 GB RAM, 2 TB SSD.</i><br />" .

			"<br />" .
			"xnget stock4~ 1000 stock4~</pre>",

			"<pre>" .
"select key, val
from table
using index ABCD
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4
limit [number]

...or...

select key, val
from table
using index ABCD
where
   index_field1 = 'index_value1' and
   index_field2 = 'index_value2'
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4
limit [number]

...or...

select key, val
from table
using index ABCD
where
   index_field1 = 'index_value1' and
   index_field2 = 'index_value2' and
   index_field3 = 'index_value3'
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4
limit [number]

...or...

select key, val
from table
using index ABCD
where
   index_field1 = 'index_value1' and
   index_field2 = 'index_value2' and
   index_field3 = 'index_value3' and
   index_field4 = 'index_value4'
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4
limit [number]</pre>"
	),

	new Cmd(
			"IX5RANGE",

			"IX5RANGE key index_name index_value1 index_value2 index_value3 index_value4 index_value5 number start",

			"Gets all subkeys stored in the <i>index_name</i> with value <i>index_value1</i>, <i>index_value2</i>, <i>index_value3</i>, <i>index_value4</i> and <i>index_value5</i>.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"<i>index_name</i> are the permutations of 5 elements, e.g. \"ABCDE\", \"ABDCE\", \"ACBDE\", \"ACDBE\", \"ADBCE\", \"ADCBE\" ...<br />" .
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
			"<i>Suppose we are selling laptops</i><br />" .
			"<br />" .
			"ix5add stock5 zen    ASUS i5 13 16 1 'Asus Zenbook,  Intel i7, 13 inch display, 16 GB RAM, 1 TB SSD' <br />" .

			"ix5add stock5 xps13  DELL i7 13 16 1 'Dell XPS13 13, Intel i7, 13 inch display, 16 GB RAM, 1 TB SSD' <br />" .

			"ix5add stock5 xps15  DELL i5 15 16 1 'Dell XPS15 15, Intel i7, 15 inch display, 16 GB RAM, 1 TB SSD' <br />" .
			"ix5add stock5 xps15b DELL i7 15 32 2 'Dell XPS15 15, Intel i7, 15 inch display, 32 GB RAM, 1 TB SSD' <br />" .
			"ix5add stock5 xps15c DELL i7 15 16 2 'Dell XPS15 15, Intel i7, 15 inch display, 16 GB RAM, 2 TB SSD' <br />" .
			"ix5add stock5 xps15d DELL i9 15 32 2 'Dell XPS15 15, Intel i7, 15 inch display, 32 GB RAM, 2 TB SSD' <br />" .

			"ix5add stock5 xps17  DELL i9 17 32 2 'Dell XPS17 17, Intel i7, 17 inch display, 32 GB RAM, 2 TB SSD' <br />" .

			"ix5range stock5 ABCDE ''   '' '' '' '' 1000 ''      <i>get all laptops sorted by brand, cpu, screen size, RAM, disk size.</i><br />" .
			"ix5range stock5 ABCDE DELL '' '' '' '' 1000 ''      <i>get all laptops from DELL.</i><br />" .
			"ix5range stock5 ABCDE DELL i7 '' '' '' 1000 ''      <i>get all laptops from DELL, CPU i7, 15 inch display.</i><br />" .
			"ix5range stock5 ABCDE DELL i7 15 '' '' 1000 ''      <i>get all laptops from DELL, CPU i7, 15 inch display.</i><br />" .
			"ix5range stock5 ABCDE DELL i7 15 16 '' 1000 ''      <i>get all laptops from DELL, CPU i7, 15 inch display, 16 GB RAM.</i><br />" .
			"ix5range stock5 ABCDE DELL i7 15 16 2  1000 ''      <i>get all laptops from DELL, CPU i7, 15 inch display, 16 GB RAM, 2 TB SSD.</i><br />" .

			"<br />" .
			"xnget stock5~ 1000 stock5~</pre>",

			"<pre>" .
"select key, val
from table
using index ABCDE
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4,
   index_field5
limit [number]

...or...

select key, val
from table
using index ABCDE
where
   index_field1 = 'index_value1'
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4,
   index_field5
limit [number]

...or...

select key, val
from table
using index ABCDE
where
   index_field1 = 'index_value1' and
   index_field2 = 'index_value2'
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4,
   index_field5
limit [number]

...or...

select key, val
from table using
index ABCDE
where
   index_field1 = 'index_value1' and
   index_field2 = 'index_value2' and
   index_field3 = 'index_value3'
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4,
   index_field5
limit [number]

...or...

select key, val
from table
using index ABCDE
where
   index_field1 = 'index_value1' and
   index_field2 = 'index_value2' and
   index_field3 = 'index_value3' and
   index_field4 = 'index_value4'
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4,
   index_field5
limit [number]

...or...

select key, val
from table
using index ABCDE
where
   index_field1 = 'index_value1' and
   index_field2 = 'index_value2' and
   index_field3 = 'index_value3' and
   index_field4 = 'index_value4' and
   index_field5 = 'index_value5'
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4,
   index_field5
limit [number]</pre>"
	),
);

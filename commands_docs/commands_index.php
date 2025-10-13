<?php
return array(
	new Cmd(
			"IX*GET",

			"IX1GET / IX2GET / IX3GET / IX4GET / IX5GET / IX6GET key subkey",

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

			"IX1MGET / IX2MGET / IX3MGET / IX4MGET / IX5MGET / IX6MGET key subkey [subkey]...",

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

			"IX1EXISTS / IX2EXISTS / IX3EXISTS / IX4EXISTS / IX5EXISTS / IX6EXISTS key subkey",

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

			"IX1GETINDEXES / IX2GETINDEXES / IX3GETINDEXES / IX4GETINDEXES / IX5GETINDEXES / IX6GETINDEXES key subkey",

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

			"Set <i>subkey</i> with index values <i>index_value1</i> + <i>sort_value1</i> and <i>value</i> in <i>key</i>.",
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

			"Set <i>subkey</i> with index values <i>index_value1</i>, <i>index_value2</i> + <i>sort_value1</i> and <i>value</i> in <i>key</i>.",
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

			"Set <i>subkey</i> with index values <i>index_value1</i>, <i>index_value2</i>, <i>index_value3</i> + <i>sort_value1</i> and <i>value</i> in <i>key</i>.",
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

			"Set <i>subkey</i> with index values <i>index_value1</i>, <i>index_value2</i>, <i>index_value3</i>, <i>index_value4</i> + <i>sort_value1</i> and <i>value</i> in <i>key</i>.",
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

			"IX5ADD key subKey index_value1 index_value2 index_value3 index_value4 index_value5 value [subKey index_value1 index_value2 index_value3 index_value4 index_value5 value]...",

			"Set <i>subkey</i> with index values <i>index_value1</i>, <i>index_value2</i>, <i>index_value3</i>, <i>index_value4</i>, <i>index_value5</i> + <i>sort_value1</i> and <i>value</i> in <i>key</i>.",
			"OK",
			"OK",
			"1.3.8",
			"[number of items] * (READ + (1 + 2 * 120) * WRITE), TX",
			false,
			true,

			"index"
	),

	new Cmd(
			"IX6ADD",

			"IX6ADD key subKey index_value1 index_value2 index_value3 index_value4 index_value5 index_value6 value [subKey index_value1 index_value2 index_value3 index_value4 index_value5 index_value6 value]...",

			"Set <i>subkey</i> with index values <i>index_value1</i>, <i>index_value2</i>, <i>index_value3</i>, <i>index_value4</i>, <i>index_value5</i>, <i>index_value6</i> + <i>sort_value1</i> and <i>value</i> in <i>key</i>.",
			"OK",
			"OK",
			"1.3.8",
			"[number of items] * (READ + (1 + 2 * 720) * WRITE), TX",
			false,
			true,

			"index"
	),

	new Cmd(
			"IX*REM",

			"IX1REM / IX2REM / IX3REM / IX4REM / IX5REM / IX6REM key subkey [subkey]...<br />" .
			"IX1REMOVE / IX2REMOVE / IX3REMOVE / IX4REMOVE / IX5REMOVE / IX6REMOVE key subkey [subkey]...<br />" .
			"IX1DEL / IX2DEL / IX3DEL / IX4DEL / IX5DEL / IX6DEL key subkey [subkey]...",

			"Removes <i>subkey</i> stored in <i>key</i>.",
			"bool",
			"Always return 1",
			"1.3.8",
			"[number of items] * (READ + { 1+1, 1+2, 1+6, 1+24, 1+120, 1+720 } * WRITE), TX",
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
"ix1add users1 john   LA x data_for_john
ix1add users1 peter  LA x data_for_peter
ix1add users1 jill   LA x data_for_jill

ix1add users1 robert NY x data_for_robert
ix1add users1 pola   NY x data_for_pola
ix1add users1 pepe   NY x data_for_pepe

ix1range users1 A ''   1000 ''          <i>get all users.</i><br />
ix1range users1 A 'LA' 1000 ''          <i>get all users from LA (john, peter, jill).</i>

xnget users1~ 1000 users1~</pre>",

			"<pre>" .
"select key, val
from table
using index A
order by
   index_field1,
   sort_value1
limit [number]

...or...

select key, val
from table
using index A
where
   index_field1 = 'index_value1'
order by
   index_field1,
   sort_value1
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
"ix2add users2 john   LA sales   x data_for_john
ix2add users2 peter  LA sales   x data_for_peter
ix2add users2 jill   LA support x data_for_jill

ix2add users2 robert NY sales   x data_for_robert
ix2add users2 pola   NY support x data_for_pola
ix2add users2 pepe   NY support x data_for_pepe

ix2range users2 AB ''      ''      1000 ''      <i>get all users sorted by city,department.</i>
ix2range users2 AB LA      ''      1000 ''      <i>get all users from LA (john, peter, jill).</i>
ix2range users2 AB LA      support 1000 ''      <i>get all users from LA, department support (jill).</i>

ix2range users2 BA ''      ''      1000 ''      <i>get all users sorted by department,city.</i>
ix2range users2 BA support ''      1000 ''      <i>get all users from department support (jill, pepe, pola).</i>
ix2range users2 BA support LA      1000 ''      <i>get all users from department support, from LA (jill).</i>

xnget users2~ 1000 users2~</pre>",

			"<pre>" .
"select key, val
from table
using index AB
order by
   index_field1,
   index_field2,
   sort_value1
limit [number]

...or...

select key, val
from table
using index AB
where
   index_field1 = 'index_value1'
order by
   index_field1,
   index_field2,
   sort_value1
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
   index_field2,
   sort_value1
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
			"<i>Suppose we are selling laptops</i><br />" .
			"<br />" .
"ix3add stock3 zen_a   ASUS AMD   Ryzen x 'Asus Zenbook,  AMD Ryzen,16 GB RAM, 1 TB SSD, 13 inch display'
ix3add stock3 zen_b   ASUS Intel i7    x 'Asus Zenbook,  Intel i7, 16 GB RAM, 1 TB SSD, 13 inch display'

ix3add stock3 xps13   DELL Intel i7    x 'Dell XPS13 13, Intel i7, 16 GB RAM, 1 TB SSD, 13 inch display'

ix3add stock3 xps15_a DELL Intel i5    x 'Dell XPS15 15, Intel i5, 16 GB RAM, 1 TB SSD, 15 inch display'
ix3add stock3 xps15_b DELL Intel i7    x 'Dell XPS15 15, Intel i7, 32 GB RAM, 1 TB SSD, 15 inch display'
ix3add stock3 xps15_c DELL Intel i7    x 'Dell XPS15 15, Intel i7, 16 GB RAM, 2 TB SSD, 15 inch display'
ix3add stock3 xps15_d DELL Intel i9    x 'Dell XPS15 15, Intel i9, 32 GB RAM, 2 TB SSD, 15 inch display'

ix3add stock3 xps17   DELL Intel i9    x 'Dell XPS17 17, Intel i7, 32 GB RAM, 2 TB SSD, 17 inch display'

ix3range stock3 ABC ''   ''    '' 1000 ''      <i>get all laptops sorted by brand, CPU brand, CPU, RAM.</i>
ix3range stock3 ABC DELL ''    '' 1000 ''      <i>get all laptops from DELL.</i>
ix3range stock3 ABC DELL Intel '' 1000 ''      <i>get all laptops from DELL, Intel CPU.</i>
ix3range stock3 ABC DELL Intel i7 1000 ''      <i>get all laptops from DELL, Intel CPU, i7 CPU.</i>

xnget stock3~ 1000 stock3~</pre>",

			"<pre>" .
"select key, val
from table
using index ABC
order by
order by
   index_field1,
   index_field2,
   index_field3,
   sort_value1
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
   index_field3,
   sort_value1
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
   index_field3,
   sort_value1
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
   index_field3,
   sort_value1
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
"ix4add stock4 zen_a   ASUS AMD   Ryzen 16 x 'Asus Zenbook,  AMD Ryzen,16 GB RAM, 1 TB SSD, 13 inch display'
ix4add stock4 zen_b   ASUS Intel i7    16 x 'Asus Zenbook,  Intel i7, 16 GB RAM, 1 TB SSD, 13 inch display'

ix4add stock4 xps13   DELL Intel i7    16 x 'Dell XPS13 13, Intel i7, 16 GB RAM, 1 TB SSD, 13 inch display'
					  x
ix4add stock4 xps15_a DELL Intel i5    16 x 'Dell XPS15 15, Intel i5, 16 GB RAM, 1 TB SSD, 15 inch display'
ix4add stock4 xps15_b DELL Intel i7    32 x 'Dell XPS15 15, Intel i7, 32 GB RAM, 1 TB SSD, 15 inch display'
ix4add stock4 xps15_c DELL Intel i7    16 x 'Dell XPS15 15, Intel i7, 16 GB RAM, 2 TB SSD, 15 inch display'
ix4add stock4 xps15_d DELL Intel i9    32 x 'Dell XPS15 15, Intel i9, 32 GB RAM, 2 TB SSD, 15 inch display'

ix4add stock4 xps17   DELL Intel i9    32 x 'Dell XPS17 17, Intel i7, 32 GB RAM, 2 TB SSD, 17 inch display'

ix4range stock4 ABCD ''   ''    '' '' 1000 ''      <i>get all laptops sorted by brand, CPU brand, CPU, RAM.</i>
ix4range stock4 ABCD DELL ''    '' '' 1000 ''      <i>get all laptops from DELL.</i>
ix4range stock4 ABCD DELL Intel '' '' 1000 ''      <i>get all laptops from DELL, Intel CPU.</i>
ix4range stock4 ABCD DELL Intel i7 '' 1000 ''      <i>get all laptops from DELL, Intel CPU, i7 CPU.</i>
ix4range stock4 ABCD DELL Intel i7 16 1000 ''      <i>get all laptops from DELL, Intel CPU, i7 CPU, 16 GB RAM.</i>

xnget stock4~ 1000 stock4~</pre>",

			"<pre>" .
"select key, val
from table
using index ABCD
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4,
   sort_value1
limit [number]

...or...

select key, val
from table
using index ABCD
where
   index_field1 = 'index_value1'
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4,
   sort_value1
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
   index_field4,
   sort_value1
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
   index_field4,
   sort_value1
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
   index_field4,
   sort_value1
limit [number]</pre>"
	),

	new Cmd(
			"IX5RANGE",

			"IX5RANGE key index_name index_value1 index_value2 index_value3 index_value4 index_value5 number start",

			"Gets all subkeys stored in the <i>index_name</i> with value <i>index_value1</i>, <i>index_value2</i>, <i>index_value3</i>, <i>index_value4</i> and <i>index_value5</i>.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"<i>index_name</i> are the permutations of 5 elements, e.g. \"ABCDE\", \"ABCED\", \"ABDCE\", \"ABDEC\", \"ABECD\", \"ABEDC\", \"ACBDE\", \"ACBED\", \"ACDBE\", \"ACDEB\", \"ACEBD\", \"ACEDC\", \"ADBCE\", \"ADBEC\", \"ADCBE\", \"ADCEB\", \"ADEBC\", \"ADECB\", \"AEBCD\", \"AEBDC\", \"AECBD\", \"AECDB\", \"AEDBC\", \"AEDCB\"...<br />" .
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
"ix5add stock5 zen_a   ASUS AMD   Ryzen 16 1 x 'Asus Zenbook,  AMD Ryzen,16 GB RAM, 1 TB SSD, 13 inch display'
ix5add stock5 zen_b   ASUS Intel i7    16 1 x 'Asus Zenbook,  Intel i7, 16 GB RAM, 1 TB SSD, 13 inch display'

ix5add stock5 xps13   DELL Intel i7    16 1 x 'Dell XPS13 13, Intel i7, 16 GB RAM, 1 TB SSD, 13 inch display'

ix5add stock5 xps15_a DELL Intel i5    16 1 x 'Dell XPS15 15, Intel i5, 16 GB RAM, 1 TB SSD, 15 inch display'
ix5add stock5 xps15_b DELL Intel i7    32 1 x 'Dell XPS15 15, Intel i7, 32 GB RAM, 1 TB SSD, 15 inch display'
ix5add stock5 xps15_c DELL Intel i7    16 2 x 'Dell XPS15 15, Intel i7, 16 GB RAM, 2 TB SSD, 15 inch display'
ix5add stock5 xps15_d DELL Intel i9    32 2 x 'Dell XPS15 15, Intel i7, 32 GB RAM, 2 TB SSD, 15 inch display'

ix5add stock5 xps17   DELL Intel i9    32 2 x 'Dell XPS17 17, Intel i9, 32 GB RAM, 2 TB SSD, 17 inch display'

ix5range stock5 ABCDE ''   ''    '' '' '' 1000 ''      <i>get all laptops sorted by brand, CPU brand, CPU, RAM, disk size.</i>
ix5range stock5 ABCDE DELL ''    '' '' '' 1000 ''      <i>get all laptops from DELL.</i>
ix5range stock5 ABCDE DELL Intel '' '' '' 1000 ''      <i>get all laptops from DELL, Intel CPU.</i>
ix5range stock5 ABCDE DELL Intel i7 '' '' 1000 ''      <i>get all laptops from DELL, Intel CPU, i7 CPU.</i>
ix5range stock5 ABCDE DELL Intel i7 16 '' 1000 ''      <i>get all laptops from DELL, Intel CPU, i7 CPU, 16 GB RAM.</i>
ix5range stock5 ABCDE DELL Intel i7 16 2  1000 ''      <i>get all laptops from DELL, Intel CPU, i7 CPU, 16 GB RAM, 2 TB SSD.</i>

xnget stock5~ 1000 stock5~</pre>",

			"<pre>" .
"select key, val
from table
using index ABCDE
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4,
   index_field5,
   sort_value1
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
   index_field5,
   sort_value1
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
   index_field5,
   sort_value1
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
   index_field5,
   sort_value1
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
   index_field5,
   sort_value1
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
   index_field5,
   sort_value1
limit [number]</pre>"
	),

	new Cmd(
			"IX6RANGE",

			"IX6RANGE key index_name index_value1 index_value2 index_value3 index_value4 index_value5 index_value6 number start",

			"Gets all subkeys stored in the <i>index_name</i> with value <i>index_value1</i>, <i>index_value2</i>, <i>index_value3</i>, <i>index_value4</i>, <i>index_value5</i> and <i>index_value6</i>.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"<i>index_name</i> are the permutations of 6 elements, e.g. \"ABCDEF\", \"ABCDFE\", \"ABCEDF\", \"ABCEFD\", \"ABCFDE\", \"ABCFED\", \"ABDCEF\", \"ABDCFE\", \"ABDECF\", \"ABDEFC\", \"ABDFCE\", \"ABDFEC\", \"ABECDF\", \"ABECFD\", \"ABEDCF\", \"ABEDFC\", \"ABEFCD\", \"ABEFDC\", \"ABFCDE\", \"ABFCED\", \"ABFDCE\", \"ABFDEC\", \"ABFECD\", \"ABFEDC\"...<br />" .
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
"ix6add stock6 zen_a   ASUS AMD   Ryzen 16 1 13 x 'Asus Zenbook,  AMD Ryzen,16 GB RAM, 1 TB SSD, 13 inch display'
ix6add stock6 zen_b   ASUS Intel i7    16 1 13 x 'Asus Zenbook,  Intel i7, 16 GB RAM, 1 TB SSD, 13 inch display'

ix6add stock6 xps13   DELL Intel i7    16 1 13 x 'Dell XPS13 13, Intel i7, 16 GB RAM, 1 TB SSD, 13 inch display'

ix6add stock6 xps15_a DELL Intel i5    16 1 15 x 'Dell XPS15 15, Intel i5, 16 GB RAM, 1 TB SSD, 15 inch display'
ix6add stock6 xps15_b DELL Intel i7    32 1 15 x 'Dell XPS15 15, Intel i7, 32 GB RAM, 1 TB SSD, 15 inch display'
ix6add stock6 xps15_c DELL Intel i7    16 2 15 x 'Dell XPS15 15, Intel i7, 16 GB RAM, 2 TB SSD, 15 inch display'
ix6add stock6 xps15_d DELL Intel i9    32 2 15 x 'Dell XPS15 15, Intel i7, 32 GB RAM, 2 TB SSD, 15 inch display'

ix6add stock6 xps17   DELL Intel i9    32 2 17 x 'Dell XPS17 17, Intel i9, 32 GB RAM, 2 TB SSD, 17 inch display'

ix6range stock6 ABCDEF ''   ''    '' '' '' '' 1000 ''      <i>get all laptops sorted by brand, CPU brand, CPU, RAM, disk size, display size.</i>
ix6range stock6 ABCDEF DELL ''    '' '' '' '' 1000 ''      <i>get all laptops from DELL.</i>
ix6range stock6 ABCDEF DELL Intel '' '' '' '' 1000 ''      <i>get all laptops from DELL, Intel CPU.</i>
ix6range stock6 ABCDEF DELL Intel i7 '' '' '' 1000 ''      <i>get all laptops from DELL, Intel CPU, i7 CPU.</i>
ix6range stock6 ABCDEF DELL Intel i7 16 '' '' 1000 ''      <i>get all laptops from DELL, Intel CPU, i7 CPU, 16 GB RAM.</i>
ix6range stock6 ABCDEF DELL Intel i7 16 2  '' 1000 ''      <i>get all laptops from DELL, Intel CPU, i7 CPU, 16 GB RAM, 2 TB SSD.</i>
ix6range stock6 ABCDEF DELL Intel i7 16 2  15 1000 ''      <i>get all laptops from DELL, Intel CPU, i7 CPU, 16 GB RAM, 2 TB SSD, 15' display.</i>

xnget stock6~ 1000 stock6~</pre>",

			"<pre>" .
"select key, val
from table
using index ABCDE
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4,
   index_field5,
   sort_value1
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
   index_field5,
   index_field6,
   sort_value1
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
   index_field5,
   index_field6,
   sort_value1
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
   index_field5,
   index_field6,
   sort_value1
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
   index_field5,
   index_field6,
   sort_value1
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
   index_field5,
   index_field6,
   sort_value1
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
   index_field5 = 'index_value5' and
   index_field6 = 'index_value6'
order by
   index_field1,
   index_field2,
   index_field3,
   index_field4,
   index_field5,
   index_field6,
   sort_value1
limit [number]</pre>"
	),
);

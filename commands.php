<?php
class Cmd{
	private $id		;
	private $name		;
	private $description	;
	private $value_type	;
	private $value		;

	private $version	;
	private $complexity	;
	private $compatible	;
	private $mutable	;
	private $module	;

	function __construct($id, $name, $description, $value_type, $value, $version, $complexity, $compatible, $mutable, $module){
		$this->id		= $id			;
		$this->name		= $name			;
		$this->description	= $description		;
		$this->value_type	= $value_type		;
		$this->value		= $value		;

		$this->version		= $version		;
		$this->complexity	= $complexity		;
		$this->compatible	= $compatible		;
		$this->mutable		= $mutable		;
		$this->module		= "$module.h"	;
	}

	static function yn($b){
		if ($b === null)
			return "n/a";

		return $b ? "Yes" : "No";
	}

	function printBody(){
		?>
		<div class="cmd" id="<?=$this->id ?>">

			<h2><?=$this->name ?></h2>

			<table border="1">
				<tr><td>Command			</td><td><?=$this->id				?></td></tr>
				<tr><td>Available since		</td><td><?=$this->version			?></td></tr>
				<tr><td>Data lookup complexity	</td><td><?=$this->complexity			?></td></tr>
				<tr><td>Redis compatible	</td><td><?=self::yn($this->compatible	)	?></td></tr>
				<tr><td>Mutable			</td><td><?=self::yn($this->mutable	)	?></td></tr>
				<tr><td>Module			</td><td><?=$this->module			?></td></tr>
			</table>

			<h3>Description:</h3>
			<?=$this->description ?>

			<h3>Return type:</h3>
			<?=$this->value_type ?>

			<h3>Return value:</h3>
			<?=$this->value ?>

			<p class="top"><a href="#top">top</a></p>
		</div>
		<?php
	}

	function printRef(){
		?>
		<li><a href="#<?=$this->id ?>"><?=$this->id ?></a></li>
		<?php
	}
};

?><html>
<head>
	<title>HM4 - command reference</title>

	<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/normalize/8.0.1/normalize.min.css.map" crossorigin="anonymous" />
	<!-- link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.1.1/css/all.min.css"    crossorigin="anonymous" / -->

	<style><!--
	pre {
		border:		dashed 1px black;
		padding:	10px;
	}

	a {
		font-weight:	bold;
		text-decoration: none;
		color:		#007;
	}

	div.cmd {
		margin-top:	30px;
		margin-bottom:	30px;
	}

	div.cmd h2 {
		--border-top:	solid 1px black;
		border-bottom:	solid 1px black;

		--background-color: #aaa;
		padding-top:	30px;
		padding-bottom:	10px;
	}

	div.cmd p.top {
		border-top:	solid 1px black;
		padding-top:	10px;
		padding-bottom:	10px;
	}

	div.cmd i {
		background-color: #ddd;
		padding-left:	10px;
		padding-right:	10px;
		font-style:	normal;
		font-weight:	bold;
		border:		dotted 1px black;
	}

	div.cmd table,
	table.x {
		border:		solid 3px black;
		border-collapse: collapse;
	}

	div.cmd table td {
		padding:	10px;
	}

	div.menurow {
		display:	flex;
		flex-wrap:	wrap;
	}

	div.menurow div {
		flex:		200px;

		border:		dotted 1px #007;

		margin:		3px;
		padding:	10px;

		vertical-align:	top;
	}

	--></style>
</head>
<body>

<a target="#top"></a>

<h1>HM4 - command reference</h1>

<h2>List of commands</h2>


<div class="menurow">
	<?php foreach(getData() as $module => $data) : ?>
		<div>
			<b><?=$module ?>.h</b><br />
			&nbsp;

			<?php
			foreach($data as $c)
				$c->printRef();
			?>
		</div>
	<?php endforeach ?>
</div>



<?php
foreach(getData() as $module => $data)
	foreach($data as $c)
		$c->printBody();
?>



</body>
</html>

<?php

function getData(){
	return array(



		"immutable" => array(



			new Cmd(
					"GET",

					"GET key",

					"Get value of the <i>key</i>. Exact match.",

					"string",
					"Value of the key or empty string.",
					"1.0.0",
					"Mem + N * Disk",
					true,
					false,

					"immutable"
			),

			new Cmd(
					"MGET",

					"MGET key1 [key2]...",

					"Get value of the <i>key1</i>, <i>key2</i>... Exact match.<br />" .
					"Same as GET but array responce.",

					"array",
					"Value of the key or empty string.",
					"1.0.0",
					"[number of keys] * (Mem + N * Disk)",
					true,
					false,

					"immutable"
			),

			new Cmd(
					"EXISTS",

					"EXISTS key",

					"Get information if <i>key</i> exists. Exact match.<br />" .
					"Operation is not recommended, because it is as fast as if you get the <i>key</i> itself.",

					"bool",
					"0 if the key value pair do not exists.<br />" .
					"1 if the key value pair exists.",
					"1.2.16",
					"Mem + N * Disk",
					true,
					false,

					"immutable"
			),

			new Cmd(
					"TTL",

					"TTL key",

					"Get TTL value of the <i>key</i>. Exact match.",

					"int",
					"Value of the TTL or 0 if there is no expiration set.",

					"1.2.11",
					"Mem + N * Disk",
					true,
					false,

					"immutable"
			),

			new Cmd(
					"STRLEN",

					"STRLEN / SIZE key",

					"Get size of the value of the <i>key</i>. Exact match.<br />" .
					"Useful for debuging HLL and BIT commands.",

					"int",
					"Size of the value TTL or 0 if key does not exists.",

					"1.2.17",
					"Mem + N * Disk",
					true,
					false,

					"immutable"
			),



		),
		"getx" => array(



			new Cmd(
					"GETX",

					"GETX key number prefix",

					"Gets <i>number</i> of key-value pairs after <i>key</i>.<br />" .
					"Returns ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
					"Returns up to 1'000 elements.<br />" .
					"<br />" .
					"<u>Example:</u><br />" .
					"<br />" .
					"<pre>set u:001:name  John<br />" .
					"set u:001:city  LA<br />" .
					"set u:001:state CA<br />" .
					"set u:001:phone 1.800.12345678<br />" .
					"getx u:001: 1000 u:001:</pre>",

					"array",
					"First group of element         - array of key and values.<br />" .
					"Second group of single element - Last key, if there is second page.",

					"1.0.0",
					"Mem + N * Disk",
					false,
					false,

					"getx"
			),

			new Cmd(
					"DELX",

					"DELX key prefix",

					"Delete up to 10'000 key-value pairs after <i>key</i>.<br />" .
					"Delete ONLY valid pairs, and only if they are matching the <i>prefix</i>.<br />" .
					"<br />" .
					"<u>Example:</u><br />" .
					"<br />" .
					"<pre>set u:001:name  John<br />" .
					"set u:001:city  LA<br />" .
					"set u:001:state CA<br />" .
					"set u:001:phone 1.800.12345678<br />" .
					"getx u:001: 1000 u:001:<br />" .
					"delx u:001:      u:001:</pre>",

					"string",
					"Last key, if there is second page.",

					"1.2.17",
					"2 * Mem + N * Disk",
					false,
					true,

					"getx"
			),



		),
		"accumulators" => array(



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
					"Mem + N * Disk",
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
					"Mem + N * Disk",
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
					"Mem + N * Disk",
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
					"Mem + N * Disk",
					false,
					false,

					"accumulators"
			),



		),
		"mutable" => array(



			new Cmd(
					"SET",

					"SET key value [seconds=0]",

					"Set <i>key</i> -> <i>value</i> pair, with optional expiration of <i>seconds</i> seconds.",

					"OK",
					"OK",
					"1.0.0",
					"Mem",
					true,
					true,

					"mutable"
			),

			new Cmd(
					"SETEX",

					"SETEX key seconds value",

					"Set <i>key</i> -> <i>value</i> pair, if key does not exists, with expiration of <i>seconds</i> seconds.<br />" .
					"Doing same as <i>SET key value seconds</i>, but in Redis-compatible way.<br />" .
					"disk.This command is used for PHP session handler.",

					"OK",
					"OK",
					"1.2.11",
					"Mem",
					true,
					true,

					"mutable"
			),

			new Cmd(
					"SETNX",

					"SETNX key value [seconds=0]",

					"Atomically Set value of the <i>key</i>, if key does not exists, with optional expiration of <i>seconds</i> seconds.<br />" .
					"Note: The command internally GET old key first.",

					"bool",
					"0 if the key value pair exists.<br />" .
					"1 if the key value pair do not exists and is set.",
					"1.2.11",
					"2 * Mem + N * Disk",
					true,
					true,

					"mutable"
			),

			new Cmd(
					"SETXX",

					"SETXX key value [seconds=0]",

					"Atomically Set value of the <i>key</i>, if key exists, with optional expiration of <i>seconds</i> seconds.<br />" .
					"Note: The command internally GET old key first.",

					"bool",
					"0 if the key value pair exists and is set.<br />" .
					"1 if the key value pair do not exists.",
					"1.2.17",
					"2 * Mem + N * Disk",
					false,
					true,

					"mutable"
			),

			new Cmd(
					"EXPIRE",

					"EXPIRE key seconds",

					"Atomically Change the expiration of the <i>key</i> to <i>seconds</i> seconds.<br />" .
					"Note: The command internally GET <i>key</i> first.<br />" .
					"If you can, use SET or SETEX instead.",

					"bool",
					"0 if the key value pair do not exists.<br />" .
					"1 if the key value pair exists and expiration is set.",
					"1.2.11",
					"2 * Mem + N * Disk",
					true,
					true,

					"mutable"
			),

			new Cmd(
					"PERSIST",

					"PERSIST key seconds",

					"Atomically remove the expiration of the <i>key</i>.<br />" .
					"Note: The command internally GET <i>key</i> first.",

					"bool",
					"0 if the key value pair do not exists.<br />" .
					"1 if the key value pair exists and expiration is removed.",
					"1.2.16",
					"2 * Mem + N * Disk",
					true,
					true,

					"mutable"
			),

			new Cmd(
					"GETSET",

					"GETSET key value [seconds=0]",

					"Gets the value of the <i>key</i>. Exact match.<br />" .
					"Then atomically Set <i>key</i> -> <i>value</i> pair.<br />" .
					"This command is often used to get value of atomic counter and reset its value to zero.",
					"string",
					"Value of the key or empty string.",
					"1.2.11",
					"2 * Mem + N * Disk",
					true,
					true,

					"mutable"
			),

			new Cmd(
					"GETDEL",

					"GETDEL key",

					"Gets the value of the <i>key</i>. Exact match.<br />" .
					"Then atomically delete the <i>key</i> -> <i>value</i> pair.<br />",
					"string",
					"Value of the key or empty string.",
					"1.2.16",
					"2 * Mem + N * Disk",
					true,
					true,

					"mutable"
			),

			new Cmd(
					"DEL",

					"DEL key [key2]... / UNLINK key [key2]...",

					"Removes <i>key</i>, <i>key2</i>...",
					"bool",
					"Always return 1",
					"1.0.0",
					"[number of keys] * Mem",
					true,
					true,

					"mutable"
			),



		),
		"copy" => array(



			new Cmd(
					"COPY",

					"COPY old_key new_key",

					"Atomically copy the <i>old_key</i> to the <i>new_key</i>.<br />" .
					"Note: The command internally GET <i>old_key</i> first.",

					"bool",
					"0 if the key value pair do not exists.<br />" .
					"1 if the key value pair exists and name is changed.",
					"1.2.16",
					"2 * Mem + N * Disk",
					true,
					true,

					"copy"
			),

			new Cmd(
					"COPYNX",

					"COPYNX old_key new_key",

					"Atomically copy the <i>old_key</i> to the <i>new_key</i>, if <i>new_key</i> does not exists.<br />" .
					"Note: The command internally GET <i>old_key</i> and <i>new_key</i> first.<br />" .
					"Note: Redis does not support this operation.",

					"bool",
					"0 if the key value pair do not exists.<br />" .
					"1 if the key value pair exists and name is changed.",
					"1.2.16",
					"3 * Mem + 2 * N * Disk",
					false,
					true,

					"copy"
			),

			new Cmd(
					"RENAME",

					"RENAME old_key new_key / MOVE old_key new_key",

					"Atomically renames <i>old_key</i> to <i>new_key</i>.",

					"bool",
					"0 if the key value pair do not exists.<br />" .
					"1 if the key value pair exists and name is changed.",
					"1.2.16",
					"2 * Mem + N * Disk",
					true,
					true,

					"copy"
			),

			new Cmd(
					"RENAMENX",

					"RENAMENX old_key new_key / MOVENX old_key new_key",

					"Atomically renames <i>old_key</i> to <i>new_key</i>, if <i>new_key</i> does not exists.<br />" .
					"Note: The command internally GET <i>old_key</i> and <i>new_key</i> first." ,

					"bool",
					"0 if the key value pair do not exists.<br />" .
					"1 if the key value pair exists and name is changed.",
					"1.2.16",
					"4 * Mem + 2 * N * Disk",
					true,
					true,

					"copy"
			),



		),
		"counter" => array(



			new Cmd(
					"INCR",

					"INCR / INCRBY key [increase value=1]",

					"Atomically increase numerical value of the <i>key</i> with <i>increase value</i>.<br />" .
					"Uses <b>int64_t</b> as a number type.",
					"string (int)",
					"New increased value.",
					"1.1.0",
					"2 * Mem + N * Disk",
					true,
					true,

					"counter"
			),

			new Cmd(
					"DECR",

					"DECR / DECRBY key [decrease value=1]",

					"Atomically decrease numerical value of the <i>key</i> with <i>decrease value</i>.<br />" .
					"Uses <b>int64_t</b> as a number type.",
					"string (int)",
					"New decrease value.",
					"1.1.0",
					"2 * Mem + N * Disk",
					true,
					true,

					"counter"
			),



		),
		"(collection module) hash" => array(



			new Cmd(
					"HGETALL",

					"HGETALL key",

					"Get <i>key</i> hash.<br />" .
					"Returns up to 1'000 elements.<br />" .
					"Works exactly as Redis HGETALL, except internally set key for each value.",
					"array",
					"array of subkey and values from key hash",
					"1.2.17",
					"Mem + N * Disk",
					true,
					true,

					"getx"
			),



			new Cmd(
					"HGET",

					"HGET key subkey",

					"Get <i>key</i> -> <i>subkey</i> hash.<br />" .
					"Works exactly as Redis HGET, except internally set key for each value.",
					"string",
					"Value of the key -> subkey hash or empty string.",
					"1.2.17",
					"Mem + N * Disk",
					true,
					true,

					"immutable"
			),



			new Cmd(
					"HMGET",

					"HMGET key subkey [subkey1]...",

					"Get <i>key</i> -> <i>subkey</i>, <i>subkey1</i>... hash.<br />" .
					"Works exactly as Redis HMGET, except internally set key for each value.<br />" .
					"If you can, use HGETALL it is much faster.",
					"array",
					"Value of the key -> subkey hash or empty string.",
					"1.2.17",
					"[number of keys] * (Mem + N * Disk)",
					true,
					true,

					"immutable"
			),



			new Cmd(
					"HEXISTS",

					"HEXISTS key subkey",

					"Get information if <i>key</i> -> <i>subkey</i> hash exists.<br /><br />" .
					"Works exactly as Redis HSET, except internally set key for each value.<br />" .
					"Operation is not recommended, because it is as fast as if you get the hash itself.",
					"bool",
					"0 if the key -> subkey hash do not exists.<br />" .
					"1 if the key -> subkey hash exists.",
					"1.2.17",
					"Mem + N * Disk",
					true,
					true,

					"immutable"
			),



			new Cmd(
					"HSET",

					"HSET key subkey val [seconds]",

					"Set <i>key</i> -> <i>subkey</i> -> <i>value</i> hash, with optional expiration of <i>seconds</i> seconds.<br />" .
					"Works exactly as Redis HSET, except internally set key for each value.",
					"OK",
					"OK",
					"1.2.17",
					"Mem",
					true,
					true,

					"mutable"
			),



			new Cmd(
					"HDEL",

					"HDEL key subkey [subkey2]...",

					"Removes <i>key</i> -> <i>subkey</i>, <i>key</i> -> <i>subkey2</i>... hash.<br />" .
					"Works exactly as Redis HSET, except internally set key for each value.",
					"bool",
					"Always return 1",
					"1.2.17",
					"[number of keys] * Mem",
					true,
					true,

					"mutable"
			),



		),
		"queue" => array(



			new Cmd(
					"SADD",

					"SADD key val [expiration]",

					"Atomically add <i>value</i> into queue <i>key</i>, with optional expiration of seconds seconds.<br />" .
					"Works exactly as Redis SADD, except internally set key for each value.",
					"OK",
					"OK",
					"1.2.16",
					"Mem",
					true,
					true,

					"queue"
			),

			new Cmd(
					"SPOP",

					"SPOP key",

					"Atomically remove single element from queue <i>key</i>.<br />" .
					"Works exactly as Redis SPOP, except internally set <b>control key</b> for each queue, also remove the key for the value.",
					"string (int)",
					"Value of the removed element or empty string.",
					"1.2.16",
					"2 * Mem + 2 * N * Disk",
					true,
					true,

					"queue"
			),



		),
		"hll" => array(



			new Cmd(
					"PFADD",

					"PFADD / HLLADD key value [value2]...",

					"Add <i>value</i>, <i>value2</i>... into HLL <i>key</i>.<br />" .
					"Read HLL information document.",
					"string (int)",
					"always returns 1",
					"1.2.17",
					"Mem + N * Disk",
					true,
					true,

					"hll"
			),

			new Cmd(
					"PFCOUNT",

					"PFCOUNT / HLLCOUNT [key1] [key2] [key3] [key4] [key5]",

					"Estimate count of HLL union of up to 5 keys.<br />" .
					"Works with single key too, but returns standard estimation.<br />" .
					"Works without key too, but returns 0.<br />" .
					"Read HLL information document.",
					"string (int)",
					"estimated count",
					"1.2.17",
					"[number of keys] * (Mem + N * Disk)",
					true,
					false,

					"hll"
			),

			new Cmd(
					"PFINTERSECT",

					"PFINTERSECT / HLLINTERSECT [key1] [key2] [key3] [key4] [key5]",

					"Estimate count of the intersection of up to 5 keys, using HLL unions.<br />" .
					"Works with single key too, but returns standard estimation.<br />" .
					"Works without key too, but returns 0.<br />" .
					"Read HLL information document.",
					"string (int)",
					"estimated count of intersection",
					"1.2.17",
					"[number of keys] * (Mem + N * Disk)",
					false,
					false,

					"hll"
			),

			new Cmd(
					"PFMERGE",

					"PFMERGE / HLLMERGE dest_key [key1] [key2] [key3] [key4] [key5]",

					"Make a HLL union of up to 5 keys and store it in <i>dest_key</i><br />" .
					"Works with single key too. Works without key too.",
					"OK",
					"Value of the removed element or empty string.",
					"1.2.17",
					"Mem + [number of keys] * (Mem + N * Disk)",
					true,
					false,

					"hll"
			),

			new Cmd(
					"PFBITS",

					"PFBITS / HLLBITS",

					"return HLL bits",
					"string (int)",
					"HLL bits",
					"1.2.17",
					"n/a",
					false,
					null,

					"hll"
			),

			new Cmd(
					"PFERROR",

					"PFERROR / HLLERROR",

					"return HLL error rate",
					"string (int)",
					"HLL error rate as percent, but multiplied to 1'00'00",
					"1.2.17",
					"n/a",
					false,
					null,

					"hll"
			),


		),
		"bitset" => array(



			new Cmd(
					"SETBIT",

					"SETBIT / BITSET key index value",

					"Set bit of the <i>key</i> with <i>index</i> to <i>value</i> (0/1).<br />" .
					"Read Bitset information document.",
					"OK",
					"OK",
					"1.2.17",
					"2 * Mem + N * Disk",
					true,
					true,

					"bitset"
			),

			new Cmd(
					"GETBIT",

					"GETBIT / BITGET key index value",

					"Get bit of the <i>key</i> with <i>index</i>.<br />" .
					"Read Bitset information document.",
					"string (int)",
					"0 / 1",
					"1.2.17",
					"Mem + N * Disk",
					true,
					false,

					"bitset"
			),

			new Cmd(
					"BITCOUNT",

					"BITCOUNT key",

					"Calculate bitcount (popcount) of the <i>key</i>,<br />" .
					"e.g. number of bits set to 1.",
					"string (int)",
					"bitcount",
					"1.2.17",
					"Mem + N * Disk",
					true,
					false,

					"bitset"
			),

			new Cmd(
					"BITMAX",

					"BITMAX",

					"Return maximum suported index,<br />" .
					"it is limited of max value size - currently 1MB * 8 bits = 8'388'608 bits.",
					"string (int)",
					"Maximum suported index.",
					"1.2.17",
					"n/a",
					false,
					null,

					"bitset"
			),



		),
		"info" => array(



			new Cmd(
					"INFO",

					"INFO",

					"Returns server information.",
					"string",
					"Server information.",
					"1.0.0",
					"n/a",
					true,
					null,

					"info"
			),

			new Cmd(
					"VERSION",

					"VERSION",

					"Returns server version.",
					"string",
					"Server version.",
					"1.2.16",
					"n/a",
					false,
					null,

					"info"
			),

			new Cmd(
					"TYPE",

					"TYPE key",

					"Returns type of given <i>key</i>.<br />" .
					"For compatibility, always return 'string'",
					"string",
					"Always return 'string'",
					"1.2.16",
					"n/a",
					true,
					null,

					"info"
			),

			new Cmd(
					"TOUCH",

					"TOUCH key",

					"Returns number of touched keys.<br />" .
					"For compatibility, always return '1'",
					"string",
					"Always return '1'",
					"1.2.17",
					"n/a",
					true,
					null,

					"info"
			),

			new Cmd(
					"PING",

					"PING",

					"Returns PONG",
					"string",
					"pong",
					"1.2.16",
					"n/a",
					true,
					null,

					"info"
			),

			new Cmd(
					"ECHO",

					"ECHO message",

					"Returns <i>message</i>.",
					"string",
					"the message",
					"1.2.16",
					"n/a",
					true,
					null,

					"info"
			),




		),
		"reload" => array(



			new Cmd(
					"SAVE",

					"SAVE / BGSAVE",

					"Flushes memtable to the disk (this is no-op on immutable servers)." .
					"Reloads the disktable(s) from the disk.",
					"OK",
					"OK",
					"1.0.0",
					"n/a",
					true,
					null,

					"reload"
			),

			new Cmd(
					"RELOAD",

					"RELOAD",

					"Reloads the disktable(s) from the disk." .
					"This command is used when disktable(s) are updated or replaced from an external process.",
					"OK",
					"OK",
					"1.0.0",
					"n/a",
					false,
					null,

					"reload"
			),



		),
		"system" => array(



			new Cmd(
					"SELECT",

					"SELECT database",

					"Provided for compatibility",
					"OK",
					"OK",
					"1.2.16",
					"n/a",
					true,
					null,

					"system"
			),

			new Cmd(
					"EXIT",

					"EXIT / QUIT",

					"Disconnect from the server.",
					"n/a",
					"n/a",
					"1.0.0",
					"n/a",
					true,
					null,

					"system"
			),

			new Cmd(
					"SHUTDOWN",

					"SHUTDOWN",

					"Shutdowns the server.<br />" .
					"SAVE / NOSAVE is not supported yet.",
					"n/a",
					"n/a",
					"1.0.0",
					"n/a",
					true,
					null,

					"system"
			),
		),
	);
}

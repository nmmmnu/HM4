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
		$this->module		= "cmd_$module.h"	;
	}

	static function yn($b){
		return $b ? "Yes" : "No";
	}

	function printBody(){
		?>
		<div class="cmd" id="<?=$this->id ?>">

			<h2><?=$this->name ?></h2>

			<h3>Description:</h3>
			<?=$this->description ?>

			<h3>Return type:</h3>
			<?=$this->value_type ?>

			<h3>Return value:</h3>
			<?=$this->value ?>

			<h3>Info:</h3>

			<table border="1" cellpadding="5">
				<tr><td>Available since		</td><td><?=$this->version			?></td></tr>
				<tr><td>Time complexity		</td><td><?=$this->complexity			?></td></tr>
				<tr><td>Redis compatible	</td><td><?=self::yn($this->compatible	)	?></td></tr>
				<tr><td>Mutable			</td><td><?=self::yn($this->mutable	)	?></td></tr>
				<tr><td>Module			</td><td><?=$this->module			?></td></tr>
			</table>

		</div>

		<p></p><a href="#top">top</a></p>
		<hr />
		<?php
	}

	function printRef(){
		?>
		<li><a href="#<?=$this->id ?>"><?=$this->id ?></a></li>
		<?php
	}
};


$cmds = array(
	/* cmd_immutable */

	new Cmd(
			"GET",

			"GET key",

			"Get value of the <i>key</i>. Exact match.",

			"string",
			"Value of the key or empty string.",
			"1.0.0",
			"O(log n)",
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
			"O(log n)",
			true,
			false,

			"immutable"
	),




	/* cmd_accumulators */

	new Cmd(
			"GETX",

			"GETX key number prefix",

			"Gets <i>number</i> of key-value pairs after <i>key</i>.<br />" .
			"Returns ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Returns up to 1'000 elements.",

			"array",
			"First group of element         - array of key and values.<br />" .
			"Second group of single element - Last key, if there is second page.",

			"1.0.0",
			"O(log n)",
			false,
			false,

			"accumulators"
	),

	new Cmd(
			"COUNT",

			"COUNT key number prefix",

			"Accumulate using COUNT <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"Accumulate ONLY valid pairs, but only if they are matching the <i>prefix</i>.<br />" .
			"Accumulate up to 10'000 elements.",

			"array",
			"First element  - count of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.4",
			"O(log n)",
			false,
			false,

			"accumulators"
	),

	new Cmd(
			"SUM",

			"SUM key number prefix",

			"Accumulate using SUM <i>number</i> key-value pairs after <i>key</i>.<br />" .
			"See COUNT for details.",

			"array",
			"First element  - sum of valid elements.<br />" .
			"Second element - last key, if there is second page.",

			"1.2.4",
			"O(log n)",
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
			"O(log n)",
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
			"O(log n)",
			false,
			false,

			"accumulators"
	),



	/* cmd_mutable */

	new Cmd(
			"SET",

			"SET key value [seconds=0]",

			"Set <i>key</i> -> <i>value</i> pair, with optional expiration of <i>seconds</i> seconds.",

			"OK",
			"OK",
			"1.0.0",
			"O(log n)",
			true,
			true,

			"mutable"
	),

	new Cmd(
			"SETEX",

			"SETEX key seconds value",

			"Set <i>key</i> -> <i>value</i> pair, if key does not exists, with optional expiration of <i>seconds</i> seconds.",

			"bool",
			"0 if key value pair do not existed.<br />" .
			"1 if the key existed and expiration is set.",
			"1.2.11",
			"O(log n) + O(1)",
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
			"0 if key value pair existed.<br />" .
			"1 if the key does not existed and is set.",
			"1.2.11",
			"O(log n) + O(1)",
			true,
			true,

			"mutable"
	),

	new Cmd(
			"EXPIRE",

			"EXPIRE key seconds",

			"Atomically Change the expiration of the <i>key</i> to <i>seconds</i> seconds.<br />" .
			"Note: The command internally GET old key first." .
			"If you can, use SET or SETEX instead.",

			"bool",
			"0 if key value pair do not existed.<br />" .
			"1 if the key existed and expiration is set.",
			"1.2.11",
			"O(log n) + O(1)",
			true,
			true,

			"mutable"
	),

	new Cmd(
			"GETSET",

			"GETSET key value [seconds=0]",

			"Gets the value of the <i>key</i>. Exact match.<br />" .
			"Then atomically Set <i>key</i> -> <i>value</i> pair." .
			"This command is often used to get value of atomic counter and reset its value to zero.",
			"string",
			"Value of the key or empty string.",
			"1.2.11",
			"O(log n) + O(1)",
			true,
			true,

			"mutable"
	),

	new Cmd(
			"DEL",

			"DEL key",

			"Removes <i>key</i>.",
			"bool",
			"Always return 1",
			"1.0.0",
			"O(1)",
			true,
			true,

			"mutable"
	),



	/* cmd_counter */

	new Cmd(
			"INCR",

			"INCR / INCRBY key [increase value=+1]",

			"Atomically increase numerical value of the <i>key</i> with <i>increase value</i>.<br />" .
			"It uses <b>int64_t</b> as a number type.",
			"string (int)",
			"New increased value.",
			"1.1.0",
			"O(log n) + O(1)",
			true,
			true,

			"counter"
	),

	new Cmd(
			"DECR",

			"DECR / DECRBY key [decrease value=+1]",

			"Atomically decrease numerical value of the <i>key</i> with <i>decrease value</i>.<br />" .
			"It uses <b>int64_t</b> as a number type.",
			"string (int)",
			"New decrease value.",
			"1.1.0",
			"O(log n) + O(1)",
			true,
			true,

			"counter"
	),



	/* cmd_info */

	new Cmd(
			"INFO",

			"INFO",

			"Returns server information.",
			"array",
			"Server information.",
			"1.0.0",
			"n/a",
			true,
			false,

			"info"
	),



	/* cmd_reload */

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
			false,

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
			true,
			false,

			"reload"
	),



	/* cmd_system */

	new Cmd(
			"EXIT",

			"EXIT",

			"Disconnect from the server.",
			"n/a",
			"n/a",
			"1.0.0",
			"n/a",
			true,
			false,

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
			false,

			"system"
	),
);
?><html>
<head>
	<title>HM4 - command reference</title>

	<!-- link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/normalize/8.0.1/normalize.min.css.map" crossorigin="anonymous" / --da>

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
		border-top:	solid 1px black;
		border-bottom:	solid 1px black;

		background-color: #aaa;
		padding:	10px;
	}

	div.cmd i {
		background-color: #ddd;
		padding-left:	10px;
		padding-right:	10px;
		font-style:	normal;
		font-weight:	bold;
		border:		dotted 1px black;
	}

	div.cmd table.db {
		border:		solid 1px #777;
		border-collapse: collapse;

		margin-top:	10px;
		margin-bottom:	10px;
	}

	div.cmd table {
		border:		solid 3px black;
		border-collapse: collapse;
	}

	div.cmd table td {
		padding:	10px;
	}

	--></style>
</head>
<body>

<a target="#top"></a>

<h1>HM4 - command reference</h1>

<h2>List of commands</h2>

<ul>
<?php
foreach($cmds as $c)
	$c->printRef();
?>
</ul>
<hr />
<?php
foreach($cmds as $c)
	$c->printBody();
?>

</body>
</html>


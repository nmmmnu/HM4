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
	private $module		;

	private $example	;

	function __construct($id, $name, $description, $value_type, $value, $version, $complexity, $compatible, $mutable, $module, $example = false, $sql = false){
		$this->id		= $id			;
		$this->name		= $name			;
		$this->description	= $description		;
		$this->value_type	= $value_type		;
		$this->value		= $value		;

		$this->version		= $version		;
		$this->complexity	= $complexity		;
		$this->compatible	= $compatible		;
		$this->mutable		= $mutable		;
		$this->module		= "$module.h"		;

		$this->example		= "$example"		;
		$this->sql		= "$sql"		;
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

			<?php if ($this->example) : ?>
			<h3>Example:</h3>
			<?=$this->example ?>
			<?php endif ?>

			<?php if ($this->sql) : ?>
			<h3>MySQL Rosetta:</h3>
			<?=$this->sql ?>
			<?php endif ?>

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

	pre i {
		color:		#A00;
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

	div.cmd > i {
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
	$d = "commands_docs";

	return array(
		"immutable"			=> require "$d/commands_immutable.php"		,
		"immutable_x"			=> require "$d/commands_immutable_x.php"	,
		"accumulators_x"		=> require "$d/commands_accumulators_x.php"	,
		"mutable"			=> require "$d/commands_mutable.php"		,
		"mutable_get"			=> require "$d/commands_mutable_get.php"	,
		"mutable_x"			=> require "$d/commands_mutable_x.php"		,
		"cas"				=> require "$d/commands_cas.php"		,
		"copy"				=> require "$d/commands_copy.php"		,
		"counter"			=> require "$d/commands_counter.php"		,
		"(collection module) hash"	=> require "$d/commands_hash.php"		,
		"queue"				=> require "$d/commands_queue.php"		,
		"bitset"			=> require "$d/commands_bitset.php"		,
		"geo"				=> require "$d/commands_geo.php"		,
		"hll"				=> require "$d/commands_hll.php"		,
		"bf"				=> require "$d/commands_bf.php"			,
		"cbf"				=> require "$d/commands_cbf.php"		,
		"cms"				=> require "$d/commands_cms.php"		,
		"index"				=> require "$d/commands_index.php"		,
		"linearcurve"			=> require "$d/commands_linearcurve.php"	,
		"mortoncurve2d"			=> require "$d/commands_mortoncurve2d.php"	,
		"mortoncurve3d"			=> require "$d/commands_mortoncurve3d.php"	,
		"mortoncurve4d"			=> require "$d/commands_mortoncurve4d.php"	,
		"mortoncurve8d"			=> require "$d/commands_mortoncurve8d.php"	,
		"mortoncurve16d"		=> require "$d/commands_mortoncurve16d.php"	,
		"hh"				=> require "$d/commands_hh.php"			,
		"mg"				=> require "$d/commands_mg.php"			,
		"rs"				=> require "$d/commands_rs.php"			,
		"tdigest"			=> require "$d/commands_tdigest.php"		,
	//	"cv"				=> require "$d/commands_cv.php"			,
		"murmur"			=> require "$d/commands_murmur.php"		,
		"info"				=> require "$d/commands_info.php"		,
		"reload"			=> require "$d/commands_reload.php"		,
		"compat"			=> require "$d/commands_compat.php"		,
		"system" 			=> require "$d/commands_system.php"		,
		"test" 				=> require "$d/commands_test.php"		,
	);
}



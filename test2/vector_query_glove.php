<?php

$redis = new Redis();
$redis->connect("127.0.0.1");

$raw		= "h";
$results	= 20;
$vst		= 0; // 0 = vsimflat, 1 = vsimlsh, 2 = vksimflat
$dist		= "c";
$distBit	= "b";



$frog = $redis->rawCommand(
	"VGETRAW"	,
	"gf300"		,
	300		,
	"f"		,
	$raw		,
	"frog"
);

//echo "$frog\n";

if (0){
vsim2(	"300 300 float"		, $redis, 1,    "gf300",  300, 300, "f", $dist,		$raw, $frog, $results);
vsim2(	"300 300 bit"		, $redis, 1,    "gb300",  300, 300, "b", $distBit,	$raw, $frog, $results);
exit;
}

vsim2(	"300 300 float"		, $redis, $vst, "gf300",  300, 300, "f", $dist,		$raw, $frog, $results);
vsim2(	"300 300 i16"		, $redis, $vst, "gs300",  300, 300, "s", $dist,		$raw, $frog, $results);
vsim2(	"300 300 i8"		, $redis, $vst, "gi300",  300, 300, "i", $dist,		$raw, $frog, $results);
vsim2(	"300 300 bit"		, $redis, $vst, "gb300",  300, 300, "b", $distBit,	$raw, $frog, $results);

vsim2(	"key 300 150 float"	, $redis, 2,    "gkf150", 300, 150, "f", $dist,		$raw, $frog, $results);

vsim2(	"300 150 float"		, $redis, $vst, "gf150",  300, 150, "f", $dist,		$raw, $frog, $results);
vsim2(	"300 150 i16"		, $redis, $vst, "gs150",  300, 150, "s", $dist,		$raw, $frog, $results);
vsim2(	"300 150 i8"		, $redis, $vst, "gi150",  300, 150, "i", $dist,		$raw, $frog, $results);
vsim2(	"300 150 bit"		, $redis, $vst, "gb150",  300, 150, "b", $distBit,	$raw, $frog, $results);

vsim2(	"key 300 150 bit"	, $redis, 2,    "gkb150", 300, 150, "b", $distBit,	$raw, $frog, $results);



$frog = $redis->rawCommand(
	"VGETRAW"	,
	"gf150"		,
	150		,
	"f"		,
	$raw		,
	"frog"
);

//echo "$frog";

vsim2(	"150 150 float"		, $redis, $vst, "gf150",  150, 150, "f", $dist,		$raw, $frog, $results);
vsim2(	"150 150 i16"		, $redis, $vst, "gs150",  150, 150, "s", $dist,		$raw, $frog, $results);
vsim2(	"150 150 i8"		, $redis, $vst, "gi150",  150, 150, "i", $dist,		$raw, $frog, $results);
vsim2(	"150 150 bit"		, $redis, $vst, "gb150",  150, 150, "b", $distBit,	$raw, $frog, $results);



function vsim2($banner, $redis, $flat, $key, $dim1, $dim2, $quant, $dist, $raw, $search, $results){
	printf("%s\n", $banner);
	printf("================================\n");
	vsim($redis, $flat, $key, $dim1, $dim2, $quant, $dist, $raw, $search, $results);
	printf("\n\n\n\n");
}



class Item{
	public string	$name;
	public float	$score;

	public function __construct(string $name, float $score){
		$this->name	= $name;
		$this->score	= $score;
	}

	public function print(){
		printf("%-30s %10.6f\n", $this->name, $this->score);
	}
}

class ItemMaxHeap extends SplMaxHeap{
	protected function compare($a, $b){
		return $a->score <=> $b->score;
	}
}

function vsim($redis, $flattype, $key, $dim1, $dim2, $quant, $dist, $raw, $search, $results){
	$heap = new ItemMaxHeap();

	switch($flattype){
	default:
	case 0: $vsim = "VSIMFLAT"	; break;
	case 1: $vsim = "VSIMLSH"	; break;
	case 2: $vsim = "VKSIMFLAT"	; break;
	}

	$startKey  = "";
	$heapCount = 0;

	$count     = 0;

	do{
		$result = $redis->rawCommand(
			$vsim		,
			$key		,
			$dim1		,
			$dim2		,
			$quant		,
			$dist		,
			$raw		,
			$search		,
			$results	,
			$startKey
		);

	//	print_r($result);

		$startKey = array_pop($result);

		for ($i = 0; $i < count($result); $i += 2){
			$name  = $result[$i];
			$score = (float) $result[$i + 1];

			if ($heapCount < $results){
				$heap->insert(new Item($name, $score));
				++$heapCount;
			}else if ($score < $heap->top()->score){
				$heap->extract();
				$heap->insert(new Item($name, $score));
			}
		}

		++$count;

	}while($startKey != "");

	$result = [];
	while (!$heap->isEmpty())
		$result[] = $heap->extract();

	$result = array_reverse($result);

	foreach($result as $item)
		$item->print();

	printf("\n");
	printf("Total %d loops...\n", $count);
	printf("\n");
}



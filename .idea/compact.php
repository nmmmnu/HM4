<?php

$in = [ 1, 1, 3, 5, 7, 10, 30, 50, 150, 399, 1000, 100000 ];

$PERCENT = 0.66;

$out = [];
$total = 0;

sort($in);

for($i = 0; $i < count($in); ++$i){
	$t = $in[$i];

	if ($t > $total * $PERCENT){
		$total += $t;
		$out[] = $i;

		continue;
	}else{
		break;
	}
}

foreach($out as $x){
	echo $x . " => " . $in[$x] . "\n";
}

echo "Total size => " . $total . "\n";

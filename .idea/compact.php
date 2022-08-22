<?php

$in = [ 1, 30, 30, 30, 30, 50, 150, 399, 1000, 100000 ];

$PERCENT = 0.9;

$out = [];
$total = 0;

sort($in);

for($i = 0; $i < count($in); ++$i){
	$t = $in[$i];

	printf("%5d %5d %5.2f\n", $t, $total, $total * $PERCENT);

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

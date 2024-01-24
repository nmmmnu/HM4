<?php

function rawCommand(){
	$args = func_get_args();

	$redis = $args[0];

	array_shift($args);

	// PHPRedis only
	return call_user_func_array( [ $redis, "rawCommand" ], $args );
}



$redis = new Redis();
$redis->connect("127.0.0.1");



// clean up.
rawCommand($redis, "xndel", "places", "places");



// add restaurants in Sofia
$redis->geoadd(
	"places",
	42.69210669738513, 23.32489318092748, "Level Up"		,
	42.69238509863727, 23.32476304836964, "Luchiano"		,
	42.69175842097124, 23.32523636032996, "Bohemian Hall"		,
	42.69269382372572, 23.32491086796810, "Umamido"			,
	42.69199298885176, 23.32411779452691, "McDonald's"		,
	42.69141486008349, 23.32517469905046, "Ceiba"			,
	42.69210516803149, 23.32626916568055, "Happy"			,
	42.69151957227103, 23.32366568780231, "Ugo"			,
	42.69151957227103, 23.32654779876320, "Il Theatro"		,
	42.69205716858902, 23.32266435016691, "Nicolas"			,
	42.68970992959111, 23.32455331244497, "Manastirska Magernitsa"	,
	42.69325388808822, 23.32118939756011, "The Hadjidragana Tavern"	,
	42.69174997126510, 23.32100561258516, "Boho"			,
	42.69205885495273, 23.33781274367048, "Stastlivetza"		,
	42.68999839888917, 23.31970366848597, "Franco's Pizza"		,
	42.69074017093236, 23.31785351529929, "Villa Rosiche"		,
	42.68342508736217, 23.31633975360111, "Chevermeto"
);



// distance from Boho to Chevermeto
// on Earth only :), in meters only

$x = $redis->geodist("places", "Boho", "Chevermeto");

// "real" distance on GoogleMaps - 1050 meters

echo $x . "\n";



// find all restaurants around some coordinates
// on Earth only :), in meters only
// not PHPRedis compatible

$x = rawCommand($redis, "georadius", "places", 42.6921, 23.3248, 100);

print_r($x);



// clean up.
rawCommand($redis, "xndel", "places", "places");



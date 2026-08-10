<?php

function cmd_MH($redis){
	rawCommand($redis, "xndel", "a", "a");

	$bands = 16;

	expect("MHADD",			rawCommand($redis, "mhadd",	"a", $bands, "doc1", ",", "apple,iphone,technology,news") == 1);
	expect("MHADD",			rawCommand($redis, "mhadd",	"a", $bands, "doc2", ",", "apple,iphone,mobile,news", "doc3", ",", "football,sports,stadium") == 1);

	// $indexes = rawCommand($redis, "mhgetindexes", "a", $bands, "doc1");
	// expect("MHGETINDEXES",		is_array($indexes) && count($indexes) == $bands);

	expect("MHJACCARD",		rawCommand($redis, "mhjaccard",	"a", $bands, "doc1", "doc1") == 1.0);
	expect("MHJACCARD",		rawCommand($redis, "mhjaccard",	"a", $bands, "doc1", "doc3") == 0.0);

	expect("MHMJACCARD",		rawCommand($redis, "mhmjaccard", "a", $bands, "doc1", "doc1", "doc3") == [ 1.0, 0.0 ] );

	expect("MHOVERLAP",		rawCommand($redis, "mhoverlap",	"a", $bands, "doc1", "doc1") == 1.0);
	expect("MHOVERLAP",		rawCommand($redis, "mhoverlap",	"a", $bands, "doc1", "doc3") == 0.0);

	expect("MHMJACCARD",		rawCommand($redis, "mhmoverlap", "a", $bands, "doc1", "doc1", "doc3") == [ 1.0, 0.0 ] );

	$sim_text = rawCommand($redis, "mhsim", "a", $bands, "doc1");
	expect("MHSIM",			is_array($sim_text) && $sim_text[0] == "doc2");

	$sim_text = rawCommand($redis, "mhsim", "a", $bands, ",", "apple,iphone,technology,news");
	expect("MHSIM",			is_array($sim_text) && $sim_text[0] == "doc1");

	expect("MHREM",			rawCommand($redis, "mhrem",	"a", $bands, "doc3") == 1);
	expect("MHREM",			rawCommand($redis, "mhrem",	"a", $bands, "doc1", "doc2") == 1);

	rawCommand($redis, "xndel", "a", "a");
}



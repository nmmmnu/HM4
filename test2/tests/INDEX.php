<?php

function cmd_INDEX($redis){
	cmd_index1($redis);
	cmd_index2($redis);
	cmd_index3($redis);
	cmd_index4($redis);
	cmd_index5($redis);
	cmd_index6($redis);
}

function cmd_index1($redis){
	rawCommand($redis, "xndel", "a", "a");

	rawCommand($redis, "ix1add", "a",
				"niki",	"sofia",	"nnn1",
				"ivan",	"varna",	"iii1",
				"boro", "pernik",	"bbb1"
	);

	expect("IX1ADD",	true									);

	expect("IX1GET",	rawCommand($redis, "ix1get",		"a", "niki"	) == "nnn1"	);
	expect("IX1GET",	rawCommand($redis, "ix1get",		"a", "ivan"	) == "iii1"	);
	expect("IX1GET",	rawCommand($redis, "ix1get",		"a", "boro"	) == "bbb1"	);

	expect("IX1MGET",	rawCommand($redis, "ix1mget",		"a",
					"niki", "ivan", "boro"	) == ["nnn1", "iii1", "bbb1"]		);

	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "niki"	)		);
	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "ivan"	)		);
	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "boro"	) 		);

	rawCommand($redis, "ix1del", "a", "boro"							);

	expect("IX1DEL",	true									);

	expect("IX1GET",	rawCommand($redis, "ix1get",		"a", "niki"	) == "nnn1"	);
	expect("IX1GET",	rawCommand($redis, "ix1get",		"a", "ivan"	) == "iii1"	);
	expect("IX1GET",	rawCommand($redis, "ix1get",		"a", "boro"	) == ""		);

	expect("IX1MGET",	rawCommand($redis, "ix1mget",		"a",
					"niki", "ivan", "boro"	) == ["nnn1", "iii1", ""]		);

	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "niki"	)		);
	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "ivan"	)		);
	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "boro"	) == false	);

	rawCommand($redis, "ix1add", "a",
				"niki",	"sofia",	"nnn2",
				"gogo",	"bristol",	"ggg2",
				"ivan",	"varna",	"iii2"
	);

	expect("IX1GET",	rawCommand($redis, "ix1get",		"a", "niki"	) == "nnn2"	);
	expect("IX1GET",	rawCommand($redis, "ix1get",		"a", "ivan"	) == "iii2"	);
	expect("IX1GET",	rawCommand($redis, "ix1get",		"a", "boro"	) == ""	);

	expect("IX1MGET",	rawCommand($redis, "ix1mget",		"a",
					"niki", "ivan", "boro"	) == ["nnn2", "iii2", ""]		);

	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "niki"	)		);
	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "ivan"	)		);
	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "boro"	) == false	);

	expect("IX1GETINDEXES",	rawCommand($redis, "ix1getindexes",	"a", "niki"	) == ["sofia"]	);
	expect("IX1GETINDEXES",	rawCommand($redis, "ix1getindexes",	"a", "ivan"	) == ["varna"]	);
	expect("IX1GETINDEXES",	rawCommand($redis, "ix1getindexes",	"a", "boro"	) == [""]	);

	expect("IX1RANGE",	rawCommand($redis, "ix1range",		"a", "A", '', 100, ''	) == [
										"gogo", "ggg2",
										"niki", "nnn2",
										"ivan", "iii2",
										""
									]				);

	expect("IX1RANGE",	rawCommand($redis, "ix1range",		"a", "A", "sofia", 100, ''	) == [
										"niki", "nnn2",
										""
									]				);

	rawCommand($redis, "xndel", "a", "a");
}

function cmd_index2($redis){
	rawCommand($redis, "xndel", "a", "a");

	rawCommand($redis, "ix2add", "a",
				"niki",	"BG",	"sofia",	"nnn1",
				"ivan",	"BG",	"varna",	"iii1",
				"boro", "BG",	"pernik",	"bbb1"
	);

	expect("IX2ADD",	true									);

	expect("IX2GET",	rawCommand($redis, "ix2get",		"a", "niki"	) == "nnn1"	);
	expect("IX2GET",	rawCommand($redis, "ix2get",		"a", "ivan"	) == "iii1"	);
	expect("IX2GET",	rawCommand($redis, "ix2get",		"a", "boro"	) == "bbb1"	);

	expect("IX2MGET",	rawCommand($redis, "ix2mget",		"a",
					"niki", "ivan", "boro"	) == ["nnn1", "iii1", "bbb1"]		);

	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "niki"	)		);
	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "ivan"	)		);
	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "boro"	) 		);

	rawCommand($redis, "ix2del", "a", "boro"							);

	expect("IX2DEL",	true									);

	expect("IX2GET",	rawCommand($redis, "ix2get",		"a", "niki"	) == "nnn1"	);
	expect("IX2GET",	rawCommand($redis, "ix2get",		"a", "ivan"	) == "iii1"	);
	expect("IX2GET",	rawCommand($redis, "ix2get",		"a", "boro"	) == ""		);

	expect("IX2MGET",	rawCommand($redis, "ix2mget",		"a",
					"niki", "ivan", "boro"	) == ["nnn1", "iii1", ""]		);

	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "niki"	)		);
	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "ivan"	)		);
	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "boro"	) == false	);

	rawCommand($redis, "ix2add", "a",
				"niki", "BG",	"sofia",	"nnn2",
				"gogo", "UK",	"bristol",	"ggg2",
				"john", "UK",	"london",	"jjj2",
				"ivan", "BG",	"varna",	"iii2"
	);

	expect("IX2GET",	rawCommand($redis, "ix2get",		"a", "niki"	) == "nnn2"	);
	expect("IX2GET",	rawCommand($redis, "ix2get",		"a", "ivan"	) == "iii2"	);
	expect("IX2GET",	rawCommand($redis, "ix2get",		"a", "boro"	) == ""		);

	expect("IX2MGET",	rawCommand($redis, "ix2mget",		"a",
					"niki", "ivan", "boro"	) == ["nnn2", "iii2", ""]		);

	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "niki"	)		);
	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "ivan"	)		);
	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "boro"	) == false	);

	expect("IX2GETINDEXES",	rawCommand($redis, "ix2getindexes",	"a", "niki"	) == ["BG", "sofia"]	);
	expect("IX2GETINDEXES",	rawCommand($redis, "ix2getindexes",	"a", "ivan"	) == ["BG", "varna"]	);
	expect("IX2GETINDEXES",	rawCommand($redis, "ix2getindexes",	"a", "boro"	) == ["", ""]		);

	expect("IX2RANGE",	rawCommand($redis, "ix2range",		"a", "AB", '', '', 100, ''	) == [
										"niki", "nnn2",
										"ivan", "iii2",
										"gogo", "ggg2",
										"john", "jjj2",
										""
									]				);

	expect("IX2RANGE",	rawCommand($redis, "ix2range",		"a", "AB", "BG", '', 100, ''	) == [
										"niki", "nnn2",
										"ivan", "iii2",
										""
									]				);

	expect("IX2RANGE",	rawCommand($redis, "ix2range",		"a", "AB", "BG", "sofia", 100, ''	) == [
										"niki", "nnn2",
										""
									]				);

	expect("IX2RANGE",	rawCommand($redis, "ix2range",		"a", "BA", "sofia", "BG", 100, ''	) == [
										"niki", "nnn2",
										""
									]				);

	rawCommand($redis, "xndel", "a", "a");
}

function cmd_index3($redis){
	rawCommand($redis, "xndel", "a", "a");

	rawCommand($redis, "ix3add", "a",
				"niki",	"BG",	"sofia",	"it",	"nnn1",
				"ivan",	"BG",	"varna",	"it",	"iii1",
				"boro", "BG",	"pernik",	"it",	"bbb1"
	);

	expect("IX3ADD",	true									);

	expect("IX3GET",	rawCommand($redis, "ix3get",		"a", "niki"	) == "nnn1"	);
	expect("IX3GET",	rawCommand($redis, "ix3get",		"a", "ivan"	) == "iii1"	);
	expect("IX3GET",	rawCommand($redis, "ix3get",		"a", "boro"	) == "bbb1"	);

	expect("IX3MGET",	rawCommand($redis, "ix3mget",		"a",
					"niki", "ivan", "boro"	) == ["nnn1", "iii1", "bbb1"]		);

	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "niki"	)		);
	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "ivan"	)		);
	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "boro"	) 		);

	rawCommand($redis, "ix3del", "a", "boro"							);

	expect("IX3DEL",	true									);

	expect("IX3GET",	rawCommand($redis, "ix3get",		"a", "niki"	) == "nnn1"	);
	expect("IX3GET",	rawCommand($redis, "ix3get",		"a", "ivan"	) == "iii1"	);
	expect("IX3GET",	rawCommand($redis, "ix3get",		"a", "boro"	) == ""		);

	expect("IX3MGET",	rawCommand($redis, "ix3mget",		"a",
					"niki", "ivan", "boro"	) == ["nnn1", "iii1", ""]		);

	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "niki"	)		);
	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "ivan"	)		);
	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "boro"	) == false	);

	rawCommand($redis, "ix3add", "a",
				"niki", "BG",	"sofia",	"it",	"nnn2",
				"gogo", "UK",	"bristol",	"it",	"ggg2",
				"john", "UK",	"london",	"hr",	"jjj2",
				"ivan", "BG",	"varna",	"hr",	"iii2"
	);

	expect("IX3GET",	rawCommand($redis, "ix3get",		"a", "niki"	) == "nnn2"	);

	expect("IX3GET",	rawCommand($redis, "ix3get",		"a", "ivan"	) == "iii2"	);
	expect("IX3GET",	rawCommand($redis, "ix3get",		"a", "boro"	) == ""		);

	expect("IX3MGET",	rawCommand($redis, "ix3mget",		"a",
					"niki", "ivan", "boro"	) == ["nnn2", "iii2", ""]		);

	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "niki"	)		);
	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "ivan"	)		);
	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "boro"	) == false	);

	expect("IX3GETINDEXES",	rawCommand($redis, "ix3getindexes",	"a", "niki"	) == ["BG", "sofia", "it"]	);
	expect("IX3GETINDEXES",	rawCommand($redis, "ix3getindexes",	"a", "ivan"	) == ["BG", "varna", "hr"]	);
	expect("IX3GETINDEXES",	rawCommand($redis, "ix3getindexes",	"a", "boro"	) == ["", "", ""]		);

	expect("IX3RANGE",	rawCommand($redis, "ix3range",		"a", "ABC", '', '', '', 100, ''	) == [
										"niki", "nnn2",
										"ivan", "iii2",
										"gogo", "ggg2",
										"john", "jjj2",
										""
									]				);

	expect("IX3RANGE",	rawCommand($redis, "ix3range",		"a", "ABC", "BG", '', '', 100, ''		) == [
										"niki", "nnn2",
										"ivan", "iii2",
										""
									]				);

	expect("IX3RANGE",	rawCommand($redis, "ix3range",		"a", "ABC", "BG", "sofia", '', 100, ''		) == [
										"niki", "nnn2",
										""
									]				);

	expect("IX3RANGE",	rawCommand($redis, "ix3range",		"a", "ABC", "BG", "sofia", "it", 100, ''	) == [
										"niki", "nnn2",
										""
									]				);

	expect("IX3RANGE",	rawCommand($redis, "ix3range",		"a", "CAB", "hr", "BG", '', 100, ''	) == [
										"ivan", "iii2",
										""
									]				);

	rawCommand($redis, "xndel", "a", "a");
}

function cmd_index4($redis){
	rawCommand($redis, "xndel", "a", "a");

	// TODO

	rawCommand($redis, "xndel", "a", "a");
}

function cmd_index5($redis){
	rawCommand($redis, "xndel", "a", "a");

	// TODO

	rawCommand($redis, "xndel", "a", "a");
}

function cmd_index6($redis){
	rawCommand($redis, "xndel", "a", "a");

	// TODO

	rawCommand($redis, "xndel", "a", "a");
}




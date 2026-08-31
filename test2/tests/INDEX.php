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
				"niki",	"sofia",	"s",
				"ivan",	"varna",	"s",
				"boro", "pernik",	"s"
	);

	expect("IX1ADD",	true									);

	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "niki"	)		);
	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "ivan"	)		);
	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "boro"	) 		);

	rawCommand($redis, "ix1rem", "a", "boro"							);

	expect("IX1REM",	true									);

	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "niki"	)		);
	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "ivan"	)		);
	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "boro"	) == false	);

	rawCommand($redis, "ix1add", "a",
				"niki",	"sofia",	"s",
				"gogo",	"bristol",	"s",
				"ivan",	"varna",	"s"
	);

	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "niki"	)		);
	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "ivan"	)		);
	expect("IX1EXISTS",	rawCommand($redis, "ix1exists",		"a", "boro"	) == false	);

	expect("IX1GETINDEXES",	rawCommand($redis, "ix1getindexes",	"a", "niki"	) == ["sofia", "s"]	);
	expect("IX1GETINDEXES",	rawCommand($redis, "ix1getindexes",	"a", "ivan"	) == ["varna", "s"]	);
	expect("IX1GETINDEXES",	rawCommand($redis, "ix1getindexes",	"a", "boro"	) == ["", ""]		);

	expect("IX1RANGE",	rawCommand($redis, "ix1range",		"a", "A", '', 100, ''	) == [
										"gogo", 1,
										"niki", 1,
										"ivan", 1,
										""
									]				);

	expect("IX1RANGE",	rawCommand($redis, "ix1range",		"a", "A", "sofia", 100, ''	) == [
										"niki", 1,
										""
									]				);

	rawCommand($redis, "xndel", "a", "a");
}

function cmd_index2($redis){
	rawCommand($redis, "xndel", "a", "a");

	rawCommand($redis, "ix2add", "a",
				"niki",	"BG",	"sofia",	"s",
				"ivan",	"BG",	"varna",	"s",
				"boro", "BG",	"pernik",	"s"
	);

	expect("IX2ADD",	true									);

	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "niki"	)		);
	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "ivan"	)		);
	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "boro"	) 		);

	rawCommand($redis, "ix2rem", "a", "boro"							);

	expect("IX2REM",	true									);

	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "niki"	)		);
	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "ivan"	)		);
	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "boro"	) == false	);

	rawCommand($redis, "ix2add", "a",
				"niki", "BG",	"sofia",	"s",
				"gogo", "UK",	"bristol",	"s",
				"john", "UK",	"london",	"s",
				"ivan", "BG",	"varna",	"s"
	);

	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "niki"	)		);
	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "ivan"	)		);
	expect("IX2EXISTS",	rawCommand($redis, "ix2exists",		"a", "boro"	) == false	);

	expect("IX2GETINDEXES",	rawCommand($redis, "ix2getindexes",	"a", "niki"	) == ["BG", "sofia", "s"]	);
	expect("IX2GETINDEXES",	rawCommand($redis, "ix2getindexes",	"a", "ivan"	) == ["BG", "varna", "s"]	);
	expect("IX2GETINDEXES",	rawCommand($redis, "ix2getindexes",	"a", "boro"	) == ["", "", ""]		);

	expect("IX2RANGE",	rawCommand($redis, "ix2range",		"a", "AB", '', '', 100, ''	) == [
										"niki", 1,
										"ivan", 1,
										"gogo", 1,
										"john", 1,
										""
									]				);

	expect("IX2RANGE",	rawCommand($redis, "ix2range",		"a", "AB", "BG", '', 100, ''	) == [
										"niki", 1,
										"ivan", 1,
										""
									]				);

	expect("IX2RANGE",	rawCommand($redis, "ix2range",		"a", "AB", "BG", "sofia", 100, ''	) == [
										"niki", 1,
										""
									]				);

	expect("IX2RANGE",	rawCommand($redis, "ix2range",		"a", "BA", "sofia", "BG", 100, ''	) == [
										"niki", 1,
										""
									]				);

	rawCommand($redis, "xndel", "a", "a");
}

function cmd_index3($redis){
	rawCommand($redis, "xndel", "a", "a");

	rawCommand($redis, "ix3add", "a",
				"niki",	"BG",	"sofia",	"it",	"s",
				"ivan",	"BG",	"varna",	"it",	"s",
				"boro", "BG",	"pernik",	"it",	"s"
	);

	expect("IX3ADD",	true									);

	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "niki"	)		);
	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "ivan"	)		);
	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "boro"	) 		);

	rawCommand($redis, "ix3rem", "a", "boro"							);

	expect("IX3REM",	true									);

	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "niki"	)		);
	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "ivan"	)		);
	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "boro"	) == false	);

	rawCommand($redis, "ix3add", "a",
				"niki", "BG",	"sofia",	"it",	"s",
				"gogo", "UK",	"bristol",	"it",	"s",
				"john", "UK",	"london",	"hr",	"s",
				"ivan", "BG",	"varna",	"hr",	"s"
	);

	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "niki"	)		);
	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "ivan"	)		);
	expect("IX3EXISTS",	rawCommand($redis, "ix3exists",		"a", "boro"	) == false	);

	expect("IX3GETINDEXES",	rawCommand($redis, "ix3getindexes",	"a", "niki"	) == ["BG", "sofia", "it", "s"]	);
	expect("IX3GETINDEXES",	rawCommand($redis, "ix3getindexes",	"a", "ivan"	) == ["BG", "varna", "hr", "s"]	);
	expect("IX3GETINDEXES",	rawCommand($redis, "ix3getindexes",	"a", "boro"	) == ["", "", "", ""]		);

	expect("IX3RANGE",	rawCommand($redis, "ix3range",		"a", "ABC", '', '', '', 100, ''	) == [
										"niki", 1,
										"ivan", 1,
										"gogo", 1,
										"john", 1,
										""
									]				);

	expect("IX3RANGE",	rawCommand($redis, "ix3range",		"a", "ABC", "BG", '', '', 100, ''		) == [
										"niki", 1,
										"ivan", 1,
										""
									]				);

	expect("IX3RANGE",	rawCommand($redis, "ix3range",		"a", "ABC", "BG", "sofia", '', 100, ''		) == [
										"niki", 1,
										""
									]				);

	expect("IX3RANGE",	rawCommand($redis, "ix3range",		"a", "ABC", "BG", "sofia", "it", 100, ''	) == [
										"niki", 1,
										""
									]				);

	expect("IX3RANGE",	rawCommand($redis, "ix3range",		"a", "CAB", "hr", "BG", '', 100, ''	) == [
										"ivan", 1,
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




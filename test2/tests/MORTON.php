<?php

function cmd_MORTON($redis){
	// clean up.
	rawCommand($redis, "xndel", "morton", "morton");



	$max_key = 4;
	$max = 50;

	// build matrix

	for($key = 0; $key < $max_key; ++$key){
		for($y = 0; $y < $max; ++$y){
			for($x = 0; $x < $max; ++$x){
				$ukey = "m:$key:$x:$y";
				$val  = "$x:$y";
				rawCommand($redis, "mc2add", "morton", $ukey, $x, $y, $val);
			}
		}
	}

	// update some
	rawCommand($redis, "mc2add", "morton",
					"m:3:5:4", 6, 5, "6:5",
					"m:3:5:5", 6, 6, "6:6"
	);

	// delete some
	rawCommand($redis, "mc2rem", "morton", "m:3:5:6");

	// get
	expect("MC2REM",	rawCommand($redis, "mc2get",		"morton", "m:3:5:4"				) == "6:5"				);
	expect("MC2GET",	rawCommand($redis, "mc2get",		"morton", "m:0:1:1"				) == "1:1"				);

	expect("MC2MGET",	rawCommand($redis, "mc2mget",		"morton", "m:0:2:2", "m:1:2:2", "nonexistent"	) == [ "2:2", "2:2", "" ]		);

	expect("MC2EXISTS",	rawCommand($redis, "mc2exists",		"morton", "m:3:5:4"				)					);
	expect("MC2EXISTS",	rawCommand($redis, "mc2exists",		"morton", "m:0:2:2"				)					);
	expect("MC2EXISTS",	rawCommand($redis, "mc2exists",		"morton", "nonexistent"				) == false				);

	expect("MC2SCORE",	rawCommand($redis, "mc2score",		"morton", "m:3:5:4"				) == [ 6, 5 ]				);
	expect("MC2SCORE",	rawCommand($redis, "mc2score",		"morton", "m:0:2:2"				) == [ 2, 2 ]				);

	$result = [
		"0000000006,0000000005,00000000", "6:5",
		"0000000006,0000000005,00000001", "6:5",
		"0000000006,0000000005,00000002", "6:5",
		"0000000006,0000000005,00000003", "6:5",
		"0000000006,0000000005,00000004", "6:5",
		""
	];

	expect("MC2POINT",	rawCommand($redis, "mc2point",		"morton", 6, 5, 1000				) == $result				);

	$result = [
		"0000000006,0000000006,00000000", "6:6",
		"0000000006,0000000006,00000001", "6:6",
		"0000000006,0000000006,00000002", "6:6",
		"0000000006,0000000006,00000003", "6:6",
		"0000000006,0000000006,00000004", "6:6",
		"0000000006,0000000007,00000005", "6:7",
		"0000000006,0000000007,00000006", "6:7",
		"0000000006,0000000007,00000007", "6:7",
		"0000000006,0000000007,00000008", "6:7",
		"0000000007,0000000006,00000009", "7:6",
		"0000000007,0000000006,0000000a", "7:6",
		"0000000007,0000000006,0000000b", "7:6",
		"0000000007,0000000006,0000000c", "7:6",
		"0000000007,0000000007,0000000d", "7:7",
		"0000000007,0000000007,0000000e", "7:7",
		"0000000007,0000000007,0000000f", "7:7",
		"0000000007,0000000007,00000010", "7:7",
		"0000000006,0000000008,00000011", "6:8",
		"0000000006,0000000008,00000012", "6:8",
		"0000000006,0000000008,00000013", "6:8",
		"0000000006,0000000008,00000014", "6:8",
		"0000000007,0000000008,00000015", "7:8",
		"0000000007,0000000008,00000016", "7:8",
		"0000000007,0000000008,00000017", "7:8",
		"0000000007,0000000008,00000018", "7:8",
		"0000000008,0000000006,00000019", "8:6",
		"0000000008,0000000006,0000001a", "8:6",
		"0000000008,0000000006,0000001b", "8:6",
		"0000000008,0000000006,0000001c", "8:6",
		"0000000008,0000000007,0000001d", "8:7",
		"0000000008,0000000007,0000001e", "8:7",
		"0000000008,0000000007,0000001f", "8:7",
		"0000000008,0000000007,00000020", "8:7",
		"0000000008,0000000008,00000021", "8:8",
		"0000000008,0000000008,00000022", "8:8",
		"0000000008,0000000008,00000023", "8:8",
		"0000000008,0000000008,00000024", "8:8",
		""
	];

	expect("MC2RANGENAIVE",	rawCommand($redis, "mc2rangenaive",	"morton", 6, 8, 6, 8, 1000		) == $result				);
	expect("MC2RANGE",	rawCommand($redis, "mc2range",		"morton", 6, 8, 6, 8, 1000		) == $result				);

	expect("MC2ENCODE",	rawCommand($redis, "mc2encode",		0x00000000, 0x00000000			) == "0000000000000000"			);
	expect("MC2ENCODE",	rawCommand($redis, "mc2encode",		       123,       5789			) == "0000000001146bdb"			);
	expect("MC2ENCODE",	rawCommand($redis, "mc2encode",		0xFFFF0000, 0x0000FFFF			) == "aaaaaaaa55555555"			);
	expect("MC2ENCODE",	rawCommand($redis, "mc2encode",		0xFFFFFFFF, 0xFFFFFFFF			) == "ffffffffffffffff"			);

	expect("MC2DECODE",	rawCommand($redis, "mc2decode",		"0000000000000000"			) == [0x00000000, 0x00000000]		);
	expect("MC2DECODE",	rawCommand($redis, "mc2decode",		"0000000001146bdb"			) == [      123,       5789]		);
	expect("MC2DECODE",	rawCommand($redis, "mc2decode",		"aaaaaaaa55555555"			) == [0xFFFF0000, 0x0000FFFF]		);
	expect("MC2DECODE",	rawCommand($redis, "mc2decode",		"ffffffffffffffff"			) == [0xFFFFFFFF, 0xFFFFFFFF]		);

	expect("MC2DECODE 0",	rawCommand($redis, "mc2decode",		"F"					) == [0x00000003, 0x00000003]		);
	expect("MC2DECODE 1",	rawCommand($redis, "mc2decode",		"FF"					) == [0x0000000F, 0x0000000F]		);
	expect("MC2DECODE 2",	rawCommand($redis, "mc2decode",		"FFF"					) == [0x0000003F, 0x0000003F]		);
	expect("MC2DECODE 3",	rawCommand($redis, "mc2decode",		"FFFF"					) == [0x000000FF, 0x000000FF]		);
	expect("MC2DECODE 4",	rawCommand($redis, "mc2decode",		"FFFFF"					) == [0x000003FF, 0x000003FF]		);
	expect("MC2DECODE 5",	rawCommand($redis, "mc2decode",		"FFFFFF"				) == [0x00000FFF, 0x00000FFF]		);
	expect("MC2DECODE 6",	rawCommand($redis, "mc2decode",		"FFFFFFF"				) == [0x00003FFF, 0x00003FFF]		);
	expect("MC2DECODE 7",	rawCommand($redis, "mc2decode",		"FFFFFFFF"				) == [0x0000FFFF, 0x0000FFFF]		);
	expect("MC2DECODE 8",	rawCommand($redis, "mc2decode",		"FFFFFFFFF"				) == [0x0003FFFF, 0x0003FFFF]		);
	expect("MC2DECODE 9",	rawCommand($redis, "mc2decode",		"FFFFFFFFFF"				) == [0x000FFFFF, 0x000FFFFF]		);
	expect("MC2DECODE A",	rawCommand($redis, "mc2decode",		"FFFFFFFFFFF"				) == [0x003FFFFF, 0x003FFFFF]		);
	expect("MC2DECODE B",	rawCommand($redis, "mc2decode",		"FFFFFFFFFFFF"				) == [0x00FFFFFF, 0x00FFFFFF]		);
	expect("MC2DECODE C",	rawCommand($redis, "mc2decode",		"FFFFFFFFFFFFF"				) == [0x03FFFFFF, 0x03FFFFFF]		);
	expect("MC2DECODE D",	rawCommand($redis, "mc2decode",		"FFFFFFFFFFFFFF"			) == [0x0FFFFFFF, 0x0FFFFFFF]		);
	expect("MC2DECODE E",	rawCommand($redis, "mc2decode",		"FFFFFFFFFFFFFFF"			) == [0x3FFFFFFF, 0x3FFFFFFF]		);
	expect("MC2DECODE F",	rawCommand($redis, "mc2decode",		"FFFFFFFFFFFFFFFF"			) == [0xFFFFFFFF, 0xFFFFFFFF]		);
	expect("MC2DECODE F",	rawCommand($redis, "mc2decode",		"FFFFFFFFFFFFFFFFF"			) == [0xFFFFFFFF, 0xFFFFFFFF]		);

	expect("MC2DECODE J",	rawCommand($redis, "mc2decode",		"junk"					) == [0x00000000, 0x00000000]		);

	// unspecified, crash test
	rawCommand($redis, "mc2decode", "FFjunk");
	rawCommand($redis, "mc2decode", "junkFF");



	rawCommand($redis, "xndel", "morton", "morton");
}


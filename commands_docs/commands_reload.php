<?php
return array(
	new Cmd(
			"LISTMAINTAINANCE",

			"LISTMAINTAINANCE",

			"Run crontab list maintainance. " .
			"For example, crontab list maintainance may fsync binlogs.<br />" .
			"Very useful if crontab list maintainance is disabled." ,
			"OK",
			"OK",
			"1.3.5.3",
			"n/a",
			false,
			null,

			"reload"
	),

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
			null,

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
			false,
			null,

			"reload"
	),
);

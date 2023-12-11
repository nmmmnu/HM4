<?php
return array(
	new Cmd(
			"CMSADD",

			"CMSADD / CMSINCR / CMSINCRBY key W_width D_depth integer_size value inc [value] [inc]",

			"Add (insert) <i>value</i>'s into the count min sketch with size <i>W_width</i> x <i>D_depth</i> and <i>integer_size</i> integers.<br />" .
			"<i>inc</i> specify how many times you want to add it / how much to increase it.<br />" .
			"Read CMS information document.",
			"OK",
			"OK",
			"1.3.1",
			"READ + WRITE",
			false,
			true,

			"cms"
	),

	new Cmd(
			"CMSRESERVE",

			"CMSRESERVE key key W_width D_depth integer_size",

			"Create new, empty count min sketch with size <i>W_width</i> x <i>D_depth</i> and <i>integer_size</i> integers.<br />" .
			"Read CMS information document.",
			"OK",
			"OK",
			"1.3.1",
			"READ + WRITE",
			false,
			true,

			"cms"
	),

	new Cmd(
			"CMSCOUNT",

			"CMSCOUNT key key W_width D_depth integer_size value",

			"Check estimated count of <i>value</i> that may be present into the count min sketch with size <i>W_width</i> x <i>D_depth</i> and <i>integer_size</i> integers.<br />" .
			"Read CMS information document.",
			"int",
			"estimated count",
			"1.3.1",
			"READ",
			false,
			false,

			"cms"
	),

	new Cmd(
			"CMSMCOUNT",

			"CMSMCOUNT / CMSQUERY key W_width D_depth integer_size value [value]",

			"Check estimated count of <i>value</i> that may be present into the count min sketch with size <i>W_width</i> x <i>D_depth</i> and <i>integer_size</i> integers.<br />" .
			"Read CMS information document.",
			"array",
			"array of estimated counts",
			"1.3.1",
			"READ",
			false,
			false,

			"cms"
	),
);

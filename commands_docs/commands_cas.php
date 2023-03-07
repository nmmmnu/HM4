<?php
return array(
	new Cmd(
			"CAS",

			"CAS key old_value new_value [seconds=0]",

			"Set the value of the <i>key</i> to <i>new_value</i>," .
			"only if the current value of the <i>key</i>  is equal to <i>old_value</i>.<br />" .
			"You can check C++ command std::compare_exchange_weak() / std::compare_exchange_strong as well.<br />" .
			"This command is compatible to Alibaba Tair Cloud.",
			"bool",
			"if the key value is set or not",
			"1.2.17",
			"READ + WRITE",
			false,
			true,

			"cas"
	),

	new Cmd(
			"CAD",

			"CAD key old_value",

			"Delete the the <i>key</i>," .
			"only if the current value of the <i>key</i>  is equal to <i>old_value</i>.<br />" .
			"This command is compatible to Alibaba Tair Cloud.",
			"bool",
			"if the key is removed or not",
			"1.2.17",
			"READ + WRITE",
			false,
			true,

			"cas"
	),
);

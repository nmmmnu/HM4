
<?php
return array(
	new Cmd(
			"ACADD_UTF8",

			"ACADD_UTF8 / ACADD key text exp",

			"Add <i>text</i> into the autocomplete object <i>key</i>, with expiration of <i>exp</i> seconds.<br />" .
			"<i>text</i> can be ASCII or UTF8.<br />" .
			"Old style encodings like CP1250, CP1251, KOI8 etc will not be tokenized correctly.",
			"int",
			"new score of the text",
			"1.3.14.2",
			"READ + (N + 1) * WRITE",
			false,
			true,

			"autocomplete"
	),
	new Cmd(
			"ACADD_BIN",

			"ACADD_BIN key text exp",

			"Add <i>text</i> into the autocomplete object <i>key</i>, with expiration of <i>exp</i> seconds.<br />" .
			"<i>text</i> can be ASCII or old style 8 bit encodings such CP1250, CP1251, KOI8 etc.<br />" .
			"UTF8 will not be tokenized correctly.",
			"int",
			"new score of the text",
			"1.3.14.2",
			"READ + (N + 1) * WRITE",
			false,
			true,

			"autocomplete"
	),
	new Cmd(
			"ACDEL_UTF8",

			"ACDEL_UTF8 / ACREM_UTF8 / ACREMOVE_UTF8 / ACDEL / ACREM / ACREMOVE key text",

			"Remove <i>text</i> from the autocomplete object <i>key</i> - UTF8 version.",
			"bool",
			"true",
			"1.3.14.2",
			"READ + (N + 1) * WRITE",
			false,
			true,

			"autocomplete"
	),
	new Cmd(
			"ACDEL_BIN",

			"ACDEL_BIN / ACREM_BIN / ACREMOVE_BIN key text",

			"Remove <i>text</i> from the autocomplete object <i>key</i> - 8 bit binary version.",
			"bool",
			"true",
			"1.3.14.2",
			"READ + (N + 1) * WRITE",
			false,
			true,

			"autocomplete"
	),

	new Cmd(
			"ACRANGE",

			"ACRANGE / ACRANGE_UTF8 / ACRANGE_BIN key text count start",

			"Make a query with <i>text</i> from the autocomplete object <i>key</i>, return up to <i>count</i> results.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"Works for ASCII, UTF8 or old style 8 bit encodings such CP1250, CP1251, KOI8 etc.",
			"bool",
			"true",
			"1.3.14.2",
			"READ",
			false,
			false,

			"autocomplete",

			"<pre>" .
			"<i>Example:</i><br />" .
			"<br />" .
"acadd a test 0
acadd a testing 0
acadd a test 0
acadd a test 0
acrange a te 100 ''
acadd a 'test now' 0
acadd a 'test now' 0
acdel a test
acrange a te 100 ''
</pre>
"
	),

	new Cmd(
			"ACRANGEALL",

			"ACRANGEALL / ACRANGEALL_UTF8 / ACRANGEALL_BIN key text count start",

			"Make a query with empty text (e.g. '') from the autocomplete object <i>key</i>, return up to <i>count</i> results.<br />" .
			"<i>start</i> specify starting key (for pagination in the similar way as in XNGET).<br />" .
			"Works for ASCII, UTF8 or old style 8 bit encodings such CP1250, CP1251, KOI8 etc.",
			"bool",
			"true",
			"1.3.14.2",
			"READ",
			false,
			false,

			"autocomplete"
	),
);


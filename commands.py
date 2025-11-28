#!/usr/bin/python3

import os
import yaml
import pystache
import pprint

DATA_MODULES	= []

MODULE_PATH	= "commands_documentation"
MODULE_METAFILE = "_module_.yaml"

def load_module(module_name):
	path = os.path.join(MODULE_PATH, module_name)
	if not os.path.isdir(path):
		print(f"Module {module_name} not found at {path}")
		return

	module = {
		"file":     module_name,
		"commands": []
	}

	file = os.path.join(path, MODULE_METAFILE)

	if os.path.isfile(file):
		with open(file, "r", encoding="utf-8") as f:
			loaded = yaml.safe_load(f)
			if isinstance(loaded, dict):
				module.update(loaded)
			else:
				print(f"{module_name}/{MODULE_METAFILE} does not contain a dict")
				return

	module_commands = []

	for fname in os.listdir(path):
		if fname.endswith(".yaml") and fname != MODULE_METAFILE:
			file = os.path.join(path, fname)
			with open(file, "r", encoding="utf-8") as f:
				command = yaml.safe_load(f)

				if isinstance(command, list):
					command[0]["file"] = fname
					module_commands.extend(command)
				else:
					print(f"{module_name}/{fname} does not contain a list")
					return


	module["commands"] = module_commands

	DATA_MODULES.append(module)

def load(path):
    with open(path, "r", encoding="utf8") as f:
        return f.read()

def generate():
	template = """
<!DOCTYPE html>
<html>
<head>
	<meta charset="utf-8" />
	<title>Commands Documentation</title>
	<style>
	a {
		font-weight:	bold;
		text-decoration: none;
		color:		#007;
	}

	div.menurow {
		display:	flex;
		flex-wrap:	wrap;
		gap:		5px;
	}

	div.menurow div {
		flex:		200px;

		border:		solid 1px #777;
		border-right:	solid 3px #777;
		border-bottom:	solid 3px #777;

		border-radius:	5px;

		padding:	10px;

		vertical-align:	top;
	}

	pre.x {
		border:		solid  1px black;
		border-left:	solid 15px #007;

		border-radius:	5px;

		padding:	20px;
	}

	table.x {
		border-collapse: collapse;
		border:		solid 2px black;
		border-right:	solid 3px black;
		border-bottom:	solid 3px black;

		margin-top:	10px;
		margin-bottom:	10px;
	}

	div.menurow div,
	table.x th {
		background-color: #eee;
	}

	table.x th,
	table.x td {
		padding:	5px;
	}
	</style>
</head>
<body>
	<a name="top"></a>
	<h1>Commands Documentation</h1>

	<h2>Modules</h2>
	<div class="menurow">
	{{#modules}}
		<div>
			<a href="#M::{{id}}">{{name}}</a>
			<ul>
			{{#commands}}
				<li><a href="#C::{{id}}">{{name}}</a></li>
			{{/commands}}
			</ul>
		</div>
	{{/modules}}
	</div>
	<hr />

	{{#modules}}
		<a name="M::{{id}}"></a>
		<h2>Module {{name}}</h2>

		{{#description}}
		<h3><b>Description:</b></h3>
		<blockquote>{{{.}}}</blockquote>
		{{/description}}

		{{#example_module}}
		<h3>Example:</h3>
		<blockquote><pre class="x">{{.}}</pre></blockquote>
		{{/example_module}}

		{{#see_also_module}}
		<h3>See Also:</h3>
		<ul>
			{{#items}}
			<li><a href="#M::{{.}}">{{.}}</a> module</li>
			{{/items}}
		</ul>
		{{/see_also_module}}

		<h3>Commands:</h3>
		<ul>
		{{#commands}}
			<li><a href="#C::{{id}}">{{name}}</a></li>
		{{/commands}}
		</ul>

		<p><a href="#top">top</a></p>
		<hr />

		{{#commands}}
		<div class="cmd" id="{{id}}">
			<a name="C::{{id}}"></a>
			<h4>{{name}}</h4>
			<table border="1" class="x">
				<tr><td>Command			</td><td>{{id}}		</td></tr>
				<tr><td>Available since		</td><td>{{version}}	</td></tr>
				{{#complexity}}
				<tr><td>Data lookup complexity	</td><td>{{.}}		</td></tr>
				{{/complexity}}
				<tr><td>Redis compatible	</td><td>{{#compatible}}Y{{/compatible}}	{{^compatible}}N{{/compatible}}		</td></tr>
				<tr><td>Mutable			</td><td>{{#mutable}}Y{{/mutable}}		{{^mutable}}N{{/mutable}}		</td></tr>
				<tr><td>Module			</td><td>{{module}}	</td></tr>
			</table>

			<h4>Usage:</h4>
			<blockquote>{{usage}}</blockquote>

			{{#params}}
			<h4>Parameters:</h4>
			<blockquote>
			<table border="1" class="x">
				{{#items}}
				<tr>
					<td>{{k}}</td>
					<td>{{v}}</td>
				</tr>
				{{/items}}
			</table>
			</blockquote>
			{{/params}}

			<h4>Description:</h4>
			<blockquote>{{{description}}}</blockquote>

			{{#return_type}}
			<h4>Return type:</h4>
			<blockquote>{{.}}</blockquote>
			{{/return_type}}

			{{#return}}
			<h4>Return value:</h4>
			<blockquote>{{{.}}}</blockquote>
			{{/return}}

			{{#aliases}}
			<h4>Aliases:</h4>
			<ul>
				{{#items}}
				<li>{{.}}</li>
				{{/items}}
			</ul>
			{{/aliases}}

			{{#example}}
			<h4>Example:</h4>
			<blockquote><pre class="x">{{.}}</pre></blockquote>
			{{/example}}

			{{#sql}}
			<h4>MySQL Rosetta:</h4>
			<blockquote><pre class="x">{{.}}</pre></blockquote>
			{{/sql}}

			{{#see_also}}
			<h4>See Also:</h4>
			<ul>
				{{#items}}
				<li><a href="#C::{{.}}">{{.}}</a></li>
				{{/items}}
			</ul>
			{{/see_also}}
		</div>

		<p><a href="#top">top</a></p>
		<hr />
		{{/commands}}
	{{/modules}}

	<h6 align="center">Copyleft 2017 - 2025, Nikolay Mihaylov<br />
	<a href="https://github.com/nmmmnu/HM4/">https://github.com/nmmmnu/HM4/</a></h6>
</body>
</html>
"""

	for mod in DATA_MODULES:
		mod["commands"].sort(key=lambda cmd: cmd['file'])

		for cmd in mod["commands"] :
			cmd["module"] = mod["name"]

	DATA = {
		"modules" : DATA_MODULES,
	#	"partials" : {
	#		'string_size_table': load(MODULE_PATH + "/_templates/string_size_table.mustache"),
	#	}
	}

	# pprint.pprint(DATA)
	# exit

	renderer = pystache.Renderer()

	output = renderer.render(template, DATA)
	print(output)





load_module("linearcurve"		)
load_module("mortoncurve2d"		)
load_module("mortoncurve3d"		)
load_module("mortoncurve4d"		)
load_module("mortoncurve8d"		)
load_module("mortoncurve16d"		)
load_module("heavy_hitters"		)
load_module("misra_gries_heavy_hitters"	)
load_module("reservoir_sampling"	)
load_module("vectors"			)
load_module("mindex"			)
load_module("autocomplete"		)
load_module("murmur"			)
load_module("random"			)
load_module("info"			)
load_module("reload"			)
load_module("compat"			)
load_module("system"			)
load_module("test"			)

generate()



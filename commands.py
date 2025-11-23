#!/usr/bin/python3

import os
import yaml
import pystache

DATA = []

MODULE_METAFILE = "_module_.yaml"

def load_module(module_name):
	""" scans commands_documentation/<module_name>/ for YAML files and adds them to all_commands"""
	path = os.path.join("commands_documentation", module_name)
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

	DATA.append(module)

def generate():
	template = """
<!DOCTYPE html>
<html>
<head>
	<meta charset="utf-8" />
	<title>Commands Documentation</title>
	<style>
	pre.x{
		padding:	20px;
		border:		dotted 1px black;
	}

	table.x{
		border-collapse: collapse;
		border:		solid 2px black;
	}

	table.x td{
		padding:	10px;
	}
	</style>
</head>
<body>
	<h1>Commands Documentation</h1>

	<h2>Modules</h2>
	<ul>
	{{#modules}}
		<li>{{name}}</li>
		<ul>
		{{#commands}}
			<li>{{name}}</li>
		{{/commands}}
		</ul>
	{{/modules}}
	</ul>
	<hr />

	{{#modules}}
		<h2>Module {{name}}</h2>

		{{#description}}
		<h3><b>Description:</b></h3>
		<blockquote>{{{.}}}</blockquote>
		{{/description}}

		{{#example}}
		<h3>Example:</h3>
		<blockquote><pre class="x">{{.}}</pre></blockquote>
		{{/example}}

		{{#see_also_module}}
		<h3>See Also:</h3>
		<ul>
			{{#items}}
			<li>{{.}} module</li>
			{{/items}}
		</ul>
		{{/see_also_module}}

		<h3>Commands:</h3>
		<ul>
		{{#commands}}
			<li>{{name}}</li>
		{{/commands}}
		</ul>

		<hr />

		{{#commands}}
		<div class="cmd" id="{{id}}">
			<h4>{{name}}</h4>
			<table border="1" class="x">
				<tr><td>Command			</td><td>{{id}}		</td></tr>
				<tr><td>Available since		</td><td>{{version}}	</td></tr>
				{{#complexity}}
				<tr><td>Data lookup complexity	</td><td>{{.}}		</td></tr>
				{{/complexity}}
				<tr><td>Redis compatible	</td><td>{{#compatible}}Y{{/compatible}}	{{^compatible}}N{{/compatible}}		</td></tr>
				<tr><td>Mutable			</td><td>{{#mutable}}{{.}}{{/mutable}}		{{^mutable}}N{{/mutable}}		</td></tr>
				<tr><td>Module			</td><td>{{module}}	</td></tr>
			</table>

			<h4>Usage:</h4>
			<blockquote>{{usage}}</blockquote>

			<h4>Description:</h4>
			<blockquote>{{{description}}}</blockquote>

			{{#return_type}}
			<h4>Return type:</h4>
			<blockquote>{{.}}</blockquote>
			{{/return_type}}

			{{#return}}
			<h4>Return value:</h4>
			<blockquote>{{.}}</blockquote>
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
				<li>{{.}}</li>
				{{/items}}
			</ul>
			{{/see_also}}
		</div>
		<hr />
		{{/commands}}
	{{/modules}}

	<h6 align="center">Copyleft 2017 - 2025, Nikolay Mihaylov<br />
	<a href="https://github.com/nmmmnu/HM4/">https://github.com/nmmmnu/HM4/</a></h6>
</body>
</html>
"""

	for mod in DATA:
		mod["commands"].sort(key=lambda cmd: cmd['file'])

		for cmd in mod["commands"] :
			cmd["module"] = mod["name"]

	renderer = pystache.Renderer()
	output = renderer.render(template, {'modules': DATA})
	print(output)


load_module("mindex"		)
load_module("autocomplete"	)

load_module("info"		)
load_module("reload"		)
load_module("compat"		)
load_module("system"		)
load_module("test"		)

generate()



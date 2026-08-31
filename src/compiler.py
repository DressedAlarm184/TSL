#!/usr/bin/env python3
import sys
import re

SIMPLE_OPS = {
	"inc": "+",
	"dec": "-",
	"next": ">",
	"prev": "<",
	"lnot": "*",
	"bnot": "~",

	"popadd": ":+",
	"popsub": ":-",
	"popmul": ":*",
	"popdiv": ":/",
	"popmod": ":%",
	"eq": ":=",
	"gt": ":>",
	"lt": ":<",
	"band": ":&",
	"bor": ":|",
	"bxor": ":^",

	"pusht": ":!",
	"pop": ":$",
	"swapt": ":X",
	"pushtp": ":t",
	"poptp": ":j",
	"pushvar": ":V",
	"popvar": ":v",

	"dup": ":D",
	"drop": ":_",
	"swap": ":x",
	"over": ":O",
	"rot": ":R",
	"roll": ":r",
	"pick": ":P",

	"putchar": "P",
	"printn": "N",
	"getchar": "#",
	"readline": "G",
	"sleep": "K",
	"rand": "?",

	"entry": "S",
	"return": "Q",
	"while": "w(",
	"forever": "f(",
	"if": "i(",
	"else": "|",
	"endblock": ")",
	"break": "E",
	"end": "%",
	"parseint": "I"
}

global_function_names = {}

def transpile_tsl(source: str, file: str) -> str:
	output = []
	allow_implicit_calls = False
	local_function_names = {}
	
	for line_num, raw_line in enumerate(source.splitlines(), start=1):
		line = re.sub(r'(#|//).*$', '', raw_line).strip()
		if not line:
			continue
			
		statements = re.split(r",\s*(?=(?:[^'\"]*['\"][^'\"]*['\"])*[^'\"]*$)", line)
		
		for stmt in statements:
			stmt = stmt.strip()
			if not stmt:
				continue
				
			match = re.match(r"^(printf|write)\s+'(.*)'$", stmt, re.IGNORECASE)
			if match:
				cmd, content = match.group(1).lower(), match.group(2)
				quote = "'" if cmd == "printf" else '"'
				output.append(f"{quote}{content}{quote}")
				continue
			elif stmt.lower().startswith(("printf", "write")):
				cmd_name = "printf" if stmt.lower().startswith("printf") else "write"
				sys.stderr.write(f"Syntax Error ({file}, line {line_num}): Invalid {cmd_name} syntax in '{stmt}'. Expected: {cmd_name} 'string'\n")
				sys.exit(1)
				
			tokens = stmt.split()
			cmd = tokens[0].lower()
			args = tokens[1:]
			
			if cmd in SIMPLE_OPS:
				output.append(SIMPLE_OPS[cmd])
				continue
				
			try:
				match cmd:
					case "incstdlib":
						try:
							with open("src/stdlib.tsle", "r") as f:
								output.append(transpile_tsl(f.read(), "stdlib"))
						except Exception as e:
							sys.stderr.write(f"Error reading standard library file: {e}\n")
							sys.exit(1)
					case "allowimplicitcalls":
						allow_implicit_calls = int(args[0])
					case "function":
						function_names = global_function_names | local_function_names
						function = function_names[args[0]]
						output.append(f"@({function["id"]},{function["tape_space"]})")
					case "define" | "export":
						local_function_names[args[0]] = {"id": int(args[1]), "tape_space": int(args[2])}
						if cmd == "export":
							global_function_names[args[0]] = local_function_names[args[0]]
					case "call":
						function_names = global_function_names | local_function_names
						func_idx = function_names[args[0]]["id"]
						output.append(f"F{func_idx:03d}")
					case "set":
						output.append(f"[{args[0]}]")
					case "add":
						output.append(f"[+{args[0]}]")
					case "sub":
						output.append(f"[-{args[0]}]")
					case "mul":
						output.append(f"[*{args[0]}]")
					case "div":
						output.append(f"[/{args[0]}]")
					case "pushi":
						output.append(f"[i{args[0]}]")
					case "addstk":
						output.append(f"[a{args[0]}]")
					case "substk":
						output.append(f"[s{args[0]}]")
					case "mulstk":
						output.append(f"[m{args[0]}]")
					case "divstk":
						output.append(f"[d{args[0]}]")
					case "setaddr":
						output.append(f"[p{args[0]}]")
					case "right":
						output.append(f"[>{args[0]}]")
					case "left":
						output.append(f"[<{args[0]}]")
					case "store":
						output.append(f".{args[0].upper()}")
					case "load":
						output.append(f",{args[0].upper()}")
					case _:
						if allow_implicit_calls:
							try:
								function_names = global_function_names | local_function_names
								func_idx = function_names[cmd]["id"]
								output.append(f"F{func_idx:03d}")
							except:
								sys.stderr.write(f"Syntax Error ({file}, line {line_num}): Unknown instruction '{stmt}'. Correct typo or define function with that name.\n")
								sys.exit(1)
						else:
							sys.stderr.write(f"Syntax Error ({file}, line {line_num}): Unknown instruction '{stmt}'\n")
							sys.exit(1)
			except IndexError:
				sys.stderr.write(f"Syntax Error ({file}, line {line_num}): Missing argument for '{cmd}'\n")
				sys.exit(1)
			except (ValueError, KeyError, AttributeError):
				sys.stderr.write(f"Syntax Error ({file}, line {line_num}): Invalid argument in '{stmt}'\n")
				sys.exit(1)
					
	return "".join(output)

def main():
	if len(sys.argv) < 3:
		sys.stderr.write(f"Usage: {sys.argv[0]} <source.tsle> <output.tsl>\n")
		sys.exit(1)

	source = sys.argv[1]
	output = sys.argv[2]
	
	try:
		with open(source, "r", encoding="utf-8") as f:
			source_content = f.read()
	except FileNotFoundError:
		sys.stderr.write(f"Error: File '{source}' not found.\n")
		sys.exit(1)
	except Exception as e:
		sys.stderr.write(f"Error reading file: {e}\n")
		sys.exit(1)

	compiled = transpile_tsl(source_content, source.rsplit("/", 1)[-1])
	
	try:
		with open(output, "w", encoding="utf-8") as f:
			f.write(compiled)
	except Exception as e:
		sys.stderr.write(f"Error writing file: {e}\n")
		sys.exit(1)

if __name__ == "__main__":
	main()

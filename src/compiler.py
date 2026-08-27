#!/usr/bin/env python3
import sys
import re

SIMPLE_OPS = {
	"inc": "+",
	"dec": "-",
	"right": ">",
	"left": "<",

	"push": "!",
	"pop": "$",
	"drop": "_",
	"swap": "X",
	"dup": "D",
	"pushaddr": ";",
	"popaddr": "&",
	"dropaddr": "\\",

	"stkadd": "^",
	"stksub": "~",
	"stkmul": "M",
	"stkdiv": "/",
	"shl": "L",
	"shr": "R",
	"eq": "=",
	"not": "*",
	"gt": "B",
	"lt": "S",
	"rand": "?",

	"putchar": "P",
	"printn": "N",
	"getchar": "#",
	"readline": "G",
	"sleep": "K",

	"entry": ":",
	"return": "Q",
	"while": "w(",
	"endblock": ")",
	"end": "%",
	"parseint": "I",
	"break": "E",
	"if": "i(",
	"else": "|",
	"function": "@",
}

def transpile_tsl(source: str) -> str:
	output = []
	
	for line_num, raw_line in enumerate(source.splitlines(), start=1):
		line = re.sub(r'(#|//).*$', '', raw_line).strip()
		if not line:
			continue
			
		statements = re.split(r",\s*(?=(?:[^'\"]*['\"][^'\"]*['\"])*[^'\"]*$)", line)
		
		for stmt in statements:
			stmt = stmt.strip()
			if not stmt:
				continue
				
			if (stmt.startswith("'") and stmt.endswith("'")) or (stmt.startswith('"') and stmt.endswith('"')):
				output.append(stmt)
				continue
				
			tokens = stmt.split()
			cmd = tokens[0].lower()
			args = tokens[1:]
			
			if cmd in SIMPLE_OPS:
				output.append(SIMPLE_OPS[cmd])
				continue
				
			try:
				match cmd:
					case "call":
						func_idx = int(args[0])
						output.append(f"F{func_idx:02d}")
					case "jump":
						func_idx = int(args[0])
						output.append(f"f{func_idx:02d}")
					case "set":
						output.append(f"[{args[0]}]")
					case "add":
						output.append(f"[+{args[0]}]")
					case "sub":
						output.append(f"[-{args[0]}]")
					case "pushimm":
						output.append(f"[i{args[0]}]")
					case "addtostk":
						output.append(f"[a{args[0]}]")
					case "subfstk":
						output.append(f"[s{args[0]}]")
					case "jumpaddr":
						output.append(f"{{{args[0]}}}")
					case "store":
						output.append(f".{args[0].upper()}")
					case "load":
						output.append(f",{args[0].upper()}")
					case _:
						sys.stderr.write(f"Syntax Error (line {line_num}): Unknown instruction '{stmt}'\n")
						sys.exit(1)
			except IndexError:
				sys.stderr.write(f"Syntax Error (line {line_num}): Missing argument for '{cmd}'\n")
				sys.exit(1)
			except ValueError:
				sys.stderr.write(f"Syntax Error (line {line_num}): Invalid argument in '{stmt}'\n")
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

	compiled = transpile_tsl(source_content)
	
	try:
		with open(output, "w", encoding="utf-8") as f:
			f.write(compiled)
	except Exception as e:
		sys.stderr.write(f"Error writing file: {e}\n")
		sys.exit(1)

if __name__ == "__main__":
	main()

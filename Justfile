build targets="library main":
	#!/bin/sh
	set -e
	mkdir -p output
	for target in {{targets}}; do
		if [ "$target" = "main" ]; then
			cc -o output/tsl source/main.c -Loutput -lTSL -Wl,-rpath,'$ORIGIN'
		elif [ "$target" = "library" ]; then
			cc -o output/libTSL.so -fPIC -shared source/interpreter.c
		elif [ "$target" = "shell" ]; then
			cc -o output/tslsh source/shell.c -Loutput -lTSL -Wl,-rpath,'$ORIGIN' -lreadline
		else
			echo "Invalid build target."
			exit 1
		fi
	done

compile input output:
	#!/bin/sh
	python3 source/compiler.py programs/{{input}} programs/{{output}}

run program:
	#!/bin/sh
	./output/tsl programs/{{program}}

shell:
	#!/bin/sh
	./output/tslsh

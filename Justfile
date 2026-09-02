build targets="library main":
	#!/bin/sh
	set -e
	for target in {{targets}}; do
		if [ "$target" = "main" ]; then
			cc -o build/tsl src/main.c -Lbuild -lTSL -Wl,-rpath,'$ORIGIN'
		elif [ "$target" = "library" ]; then
			cc -o build/libTSL.so -fPIC -shared src/interpreter.c
		elif [ "$target" = "shell" ]; then
			cc -o build/tslsh src/shell.c -Lbuild -lTSL -Wl,-rpath,'$ORIGIN' -lreadline
		else
			echo "Invalid build target."
			exit 1
		fi
	done

compile input output:
	#!/bin/sh
	python3 src/compiler.py programs/{{input}} programs/{{output}}

run program:
	#!/bin/sh
	./build/tsl programs/{{program}}

shell:
	#!/bin/sh
	./build/tslsh

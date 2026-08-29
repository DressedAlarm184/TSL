build:
	cc -o build/tsl src/main.c

compile input output:
	python3 src/compiler.py programs/{{input}} programs/{{output}}

run program:
	./build/tsl programs/{{program}}

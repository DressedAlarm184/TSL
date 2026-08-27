build:
	cc -o build/tsl src/main.c

compile input output:
	python3 src/compiler.py {{input}} {{output}}

run program:
  ./build/tsl {{program}}
  
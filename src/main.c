#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <stdint.h>
#include <string.h>

typedef struct {
	int start;
	int end;
} Loop;

typedef struct {
	int entry;
	int tape_space;
} Function;

typedef struct {
	Function* func;
	struct {int ip, tp, lp;} ret;
} CallStack;

#include "helpers.c"
#include "interpreter.c"

int main(int argc, char* argv[]) {
	srand(time(NULL));

	if (argc != 2) {
		fprintf(stderr, "Error: Missing file! Usage: %s <file.tsl>\n", argv[0]);
		return 1;
	}

	FILE* file = fopen(argv[1], "r");
	if (file == NULL) {
		perror("open");
		return 1;
	}

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	rewind(file);

	char program[size + 1];
	fread(program, 1, size, file);
	program[size] = 0;

	run_tsl_code((unsigned char*)program);

	return 0;
}

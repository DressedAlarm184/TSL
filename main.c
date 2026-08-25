#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

void run_tsl_program(char* program) {
	printw("Stub run_tsl_program function.");
}

int main(int argc, char* argv[]) {
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

	char* program = malloc(size + 1);
	fread(program, 1, size, file);
	program[size] = 0;

	initscr(), cbreak(), noecho();
	keypad(stdscr, TRUE); 

	run_tsl_program(program);

	printw("\n\nTSL program has ended. Press any key to exit...");
	getch();

	endwin();
	free(program);
}

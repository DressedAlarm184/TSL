#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

void putch(unsigned char ch) {
	if (ch == '\n' || ch == '\t' || ch == '\r') {
		addch(ch);
	} else if (!iscntrl(ch)) {
		addch(ch);
	}
}

void populate_program_layout(int* ip, int functions[], unsigned char* program) {
	char in_string = 0;

	for (int i = 0, c = 0; program[i] != 0; i++) {
		char ch = program[i];

		if (in_string == 0) {
			if (ch == '\'' || ch == '"') {
				in_string = ch;
			} else if (ch == '@') {
				functions[c++] = i;
			} else if (ch == ':' && *ip == 0) {
				*ip = i + 1;
			}
		} else {
			if (ch == in_string) in_string = 0;
		}
	}
}

void run_tsl_code(unsigned char* program) {
	int ip = 0, tp = 0, sp = 0, lp = 0, ap = 0, cp = 0;
	unsigned char user_stack[1024] = {0}, tape[8192] = {0};
	int loop_stack[64] = {0}, addr_stack[256] = {0};
	int functions[100] = {0}, call_stack[128] = {0};
	unsigned int iterations = 0;
	unsigned char variables[26] = {0};

	populate_program_layout(&ip, functions, program);

	while (program[ip] != '%' && program[ip] != 0) {
		switch (program[ip]) {
			case '+': tape[tp]++; break;
			case '-': tape[tp]--; break;
			case '>': tp++; break;
			case '<': tp--; break;
			case 'P': putch(tape[tp]); break;
			case 'N': printw("%d", tape[tp]); break;
			case 'L': tape[tp] <<= 1; break;
			case 'R': tape[tp] >>= 1; break;
			case '!': user_stack[++sp] = tape[tp]; break;
			case '$': tape[tp] = user_stack[sp--]; break;
			case '^': tape[tp] += user_stack[sp--]; break;
			case '~': tape[tp] -= user_stack[sp--]; break;
			case '#': tape[tp] = getch(); break;
			case '?': tape[tp] = rand() % (tape[tp] ? (tape[tp]+1) : 256); break;
			case '{': {
				ip++;
				unsigned short start_addr = ip;
				char nb[8] = {0};
				while (program[ip] != '}') nb[ip - start_addr] = program[ip++];
				tp = strtol(nb, NULL, 0);
				break;
			}
			case '[': {
				ip++;
				char number_buffer[8] = {0}; int buf_idx = 0; char mode = '=';
				if (program[ip] == '+' || program[ip] == '-') mode = program[ip++];
				while (program[ip] != ']') number_buffer[buf_idx++] = program[ip++];
				int val = strtol(number_buffer, NULL, 0);
				if (mode == '+') {
					tape[tp] += val;
				} else if (mode == '-') {
					tape[tp] -= val;
				} else tape[tp] = (unsigned char)val;
				break;
			}
			case 'G': {
				echo();
				getnstr((char*)&tape[tp], user_stack[sp--]);
				noecho();
				while (tape[tp] != 0) tp++;
				break;
			}
			case '"':
				ip++;
				for (; program[ip] != '"'; ip++, tp++) {
					tape[tp] = program[ip];
				}
				break;
			case '(':
				if (tape[tp] == 0) {
					int loop_balance = 1;
					while (loop_balance > 0) {
						if (program[++ip] == '(') loop_balance++;
						else if (program[ip] == ')') loop_balance--;
					}
				} else {
					loop_stack[lp++] = ip;
				}
				break;
			case ')':
				if ((tape[tp] != 0) && (lp > 0)) {
					ip = loop_stack[lp - 1];
				} else if (lp > 0) lp--;
				break;
			case 'X': {
				unsigned char temp = user_stack[sp];
				user_stack[sp] = tape[tp];
				tape[tp] = temp;
				break;
			}
			case 'K': usleep(tape[tp] * 10000); break;
			case '=': tape[tp] = (user_stack[sp--] == tape[tp]) ? 1 : 0; break;
			case '*': tape[tp] = (user_stack[sp--] != tape[tp]) ? 1 : 0; break;
			case 'B': tape[tp] = (user_stack[sp--] > tape[tp]) ? 1 : 0; break;
			case 'S': tape[tp] = (user_stack[sp--] < tape[tp]) ? 1 : 0; break;
			case '\'':
				ip++;
				while (program[ip] != '\'')	putch(program[ip++]);
				break;
			case 'M': tape[tp] *= user_stack[sp--]; break;
			case '/': tape[tp] /= user_stack[sp--]; break;
			case '&': tp = addr_stack[ap--]; break;
			case '_': sp--; break;
			case '\\': ap--; break;
			case ';': addr_stack[++ap] = tp; break;
			case '.': {
				char ch = program[++ip];
				if (ch >= 'A' && ch <= 'Z') {
					int index = ch - 'A';
					variables[index] = tape[tp];
				}
				break;
			}
			case ',': {
				char ch = program[++ip];
				if (ch >= 'A' && ch <= 'Z') {
					int index = ch - 'A';
					tape[tp] = variables[index];
				}
				break;
			}
			case 'I': {
				int parsed_value = 0;
				while (tape[tp] >= '0' && tape[tp] <= '9') {
					parsed_value = (parsed_value * 10) + (tape[tp] - '0');
					tp++;
				}
				user_stack[++sp] = (unsigned char)(parsed_value & 0xFF);
				break;
			}
			case 'Q': ip = call_stack[--cp]; break;
			case 'F': {
				int func_index = 0;
				sscanf((char*)&program[ip + 1], "%2d", &func_index);
				call_stack[cp++] = ip + 2;
				ip = functions[func_index] - 1;
				break;
			}
		}

		ip++, iterations++;
		if (iterations % 10000 == 0) usleep(1000);

		nodelay(stdscr, TRUE);
		int ch = getch();
		nodelay(stdscr, FALSE);
		if (ch == 24) goto end;

		refresh();
	}

	end:;
}

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

	char* program = malloc(size + 1);
	fread(program, 1, size, file);
	program[size] = 0;

	initscr(), cbreak(), noecho();
	keypad(stdscr, TRUE), scrollok(stdscr, TRUE);

	run_tsl_code(program);

	printw("\n\nTSL program has ended. Press any key to exit...");
	getch();

	endwin();
	free(program);
}

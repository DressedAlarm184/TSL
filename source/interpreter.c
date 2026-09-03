#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdint.h>
#include <termios.h>
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

int scan_block(char* program, int ip, char open, char close, char alt, int* found_alt);
void populate_function_list(char* program, Function* functions);

void run_tsl_code(char* program) {
	int ip = 0, tp = 0, sp = 0, lp = 0, cp = 0, ap = 0;
	uint16_t user_stack[8192] = {0}, tape[32768] = {0}, variables[26] = {0};
	CallStack call_stack[512] = {0}; Loop loop_stack[64] = {0}; Function functions[1000] = {0};
	WINDOW* ncwin = NULL; uint16_t aux_stack[128] = {0};

	populate_function_list(program, functions);

	for (; program[ip] != '%' && program[ip] != 0; ip++) switch (program[ip]) {
		case '+': tape[tp]++; break;
		case '-': tape[tp]--; break;
		case '>': tp++; break;
		case '<': tp--; break;
		case 'P': putchar(tape[tp]); fflush(stdout); break;
		case 'N': printf("%d", tape[tp]); fflush(stdout); break;
		case ':': switch (program[++ip]) {
			case '+': tape[tp] += user_stack[sp--]; break;
			case '-': tape[tp] -= user_stack[sp--]; break;
			case '*': tape[tp] *= user_stack[sp--]; break;
			case '/': tape[tp] /= user_stack[sp--]; break;
			case '%': tape[tp] %= user_stack[sp--]; break;
			case '=': tape[tp] = (user_stack[sp--] == tape[tp]) ? 1 : 0; break;
			case '>': tape[tp] = (user_stack[sp--] > tape[tp]) ? 1 : 0; break;
			case '<': tape[tp] = (user_stack[sp--] < tape[tp]) ? 1 : 0; break;
			case '!': user_stack[++sp] = tape[tp]; break;
			case '$': tape[tp] = user_stack[sp--]; break;
			case '&': tape[tp] &= user_stack[sp--]; break;
			case '|': tape[tp] |= user_stack[sp--]; break;
			case '^': tape[tp] ^= user_stack[sp--]; break;
			case 'X': {
				uint16_t temp = user_stack[sp];
				user_stack[sp] = tape[tp];
				tape[tp] = temp;
				break;
			}
			case 'x': {
				uint16_t temp = user_stack[sp];
				user_stack[sp] = user_stack[sp - 1];
				user_stack[sp - 1] = temp;
				break;
			}
			case 'O':
				user_stack[sp + 1] = user_stack[sp - 1]; sp++; break;
			case 'R': {
				uint16_t temp = user_stack[sp - 2];
				user_stack[sp - 2] = user_stack[sp - 1];
				user_stack[sp - 1] = user_stack[sp];
				user_stack[sp] = temp;
				break;
			}
			case 'r': {
				int n = user_stack[sp--];
				uint16_t item = user_stack[sp - n];
				for (int i = sp - n; i < sp; i++) {
					user_stack[i] = user_stack[i + 1];
				}
				user_stack[sp] = item;
				break;
			}
			case 'P': {
				int n = user_stack[sp--];
				uint16_t item = user_stack[sp - n];
				user_stack[++sp] = item;
				break;
			}
			case 't': user_stack[++sp] = (uint16_t)tp; break;
			case 'j': tp = user_stack[sp--]; break;
			case 'D': case 'd': user_stack[sp + 1] = user_stack[sp]; sp++; break;
			case '_': if (sp > 0) sp--; break;
			case 'V': user_stack[sp] = variables[user_stack[sp]]; break;
			case 'v': {
				int var_index = user_stack[sp--];
				variables[var_index] = user_stack[sp--];
				break;
			}
			case 'S': tape[user_stack[sp - 1]] = user_stack[sp]; sp -= 2; break;
			case 'L': user_stack[sp] = tape[user_stack[sp]]; break;
		} break;
		case ';': switch (program[++ip]) {
			case '+': user_stack[sp - 1] += user_stack[sp]; sp--; break;
			case '-': user_stack[sp - 1] -= user_stack[sp]; sp--; break;
			case '*': user_stack[sp - 1] *= user_stack[sp]; sp--; break;
			case '/': user_stack[sp - 1] /= user_stack[sp]; sp--; break;
			case '>': aux_stack[++ap] = user_stack[sp--]; break;
			case '<': user_stack[++sp] = aux_stack[ap--]; break;
		} break;
		case '#': {
			struct termios old, new;
			tcgetattr(STDIN_FILENO, &old);
			new = old, new.c_lflag &= ~(ICANON | ECHO);
			tcsetattr(STDIN_FILENO, TCSANOW, &new);
			tape[tp] = getchar();
			tcsetattr(STDIN_FILENO, TCSANOW, &old);
			break;
		}
		case '?': tape[tp] = rand() % (tape[tp] ? tape[tp] + 1 : 65536); break;
		case '[': {
			ip++;
			char number_buffer[12] = {0}; int buf_idx = 0; char mode = '=';
			if (program[ip] < '0' || program[ip] > '9') mode = program[ip++];
			while (program[ip] != ']') {
				number_buffer[buf_idx++] = program[ip++];
			}
			uint16_t val = (uint16_t)strtol(number_buffer, NULL, 0);
			switch (mode) {
				case 'p': tp = val; break;
				case '>': tp += val; break;
				case '<': tp -= val; break;
				case '=': tape[tp] = val;  break;
				case '+': tape[tp] += val; break;
				case '-': tape[tp] -= val; break;
				case '*': tape[tp] *= val; break;
				case '/': tape[tp] /= val; break;
				case 'i': user_stack[++sp] = val; break;
				case 'a': user_stack[sp] += val; break;
				case 's': user_stack[sp] -= val; break;
				case 'm': user_stack[sp] *= val; break;
				case 'd': user_stack[sp] /= val; break;
				case 'S': sleep(val); break;
			}
			break;
		}
		case 'G': {
			int start_tp = tp;
			uint16_t max_len = user_stack[sp--];
			char temp_buf[max_len + 1];
			if (fgets(temp_buf, sizeof temp_buf, stdin) == NULL)
				temp_buf[0] = '\0';
			for (int i = 0; temp_buf[i] != '\0'; ++i)
				tape[tp++] = (unsigned char)temp_buf[i];
			tape[tp] = 0;
			tp = start_tp;
			break;
		}
		case '"': {
			int target = user_stack[sp--];
			ip++;
			for (; program[ip] != '"'; ip++, target++) {
				tape[target] = program[ip];
			}
			tape[target] = 0;
			break;
		}
		case ')':
			if (lp > 0 && ip == loop_stack[lp - 1].end) {
				if (tape[tp] != 0 || program[loop_stack[lp - 1].start - 1] == 'f') {
					ip = loop_stack[lp - 1].start;
				} else {
					lp--;
				}
			}
			break;
		case '*': tape[tp] = !tape[tp]; break;
		case '~': tape[tp] = ~tape[tp]; break;
		case '\'':
			ip++;
			while (program[ip] != '\'') {
				if (program[ip] == '\\') switch (program[++ip]) {
					case 'n': putchar('\n'); break;
					case 'r': putchar('\r'); break;
					case 't': putchar('\t'); break;
					case '"': putchar('\''); break;
					case '\\': putchar('\\'); break;
					default: putchar('?'); break;
				} else if (program[ip] == '%') switch (program[++ip]) {
					case '%': putchar('%'); break;
					case 's':
						for (int temp_tp = tp; tape[temp_tp] != 0; temp_tp++) {
							putchar((char)(tape[temp_tp] & 0xFF));
						}
						break;
					case 'd': printf("%d", tape[tp]); break;
					case 'c': printf("%c", tape[tp]); break;
					case 't': printf("%d", user_stack[sp]); break;
					case 'p': printf("%d", user_stack[sp--]); break;
					default: putchar('?'); break;
				} else {
					putchar(program[ip]);
				}
				ip++;
			}
			fflush(stdout);
			break;
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
			int start_tp = tp;
			uint32_t parsed_value = 0;
			while (tape[tp] >= '0' && tape[tp] <= '9') {
				parsed_value = (parsed_value * 10) + (tape[tp] - '0');
				tp++;
			}
			user_stack[++sp] = (uint16_t)(parsed_value & 0xFFFF);
			tp = start_tp;
			break;
		}
		case 'Q': {
			CallStack* entry = &call_stack[--cp];
			if (entry->func->tape_space > 0) tp = entry->ret.tp;
			ip = entry->ret.ip, lp = entry->ret.lp;
			break;
		}
		case 'F': {
			int idx = 0, indirect = 0;
			if (program[ip + 1] == 'x') indirect = 1, idx = user_stack[sp--];
			else sscanf((char*)&program[ip + 1], "%3d", &idx);
			CallStack entry = (CallStack){.func = &functions[idx],.ret = {.ip = ip + (indirect ? 1 : 3), .tp = tp, .lp = lp}};
			call_stack[cp++] = entry, ip = entry.func->entry - 1;
			if (entry.func->tape_space > 0) {
				int new_tp = 16384;
				for (int i = 0; i < cp - 1; i++) {
					new_tp += call_stack[i].func->tape_space;
				}
				tp = new_tp;
				for (int i = 0; i < entry.func->tape_space; i++) tape[tp + i] = 0;
			}
			break;
		}
		case 'E': if (lp > 0) ip = loop_stack[--lp].end; break;
		case 'i':
			if (program[ip + 1] == '(') {
				ip++;
				if (tape[tp] == 0) {
					int found_else = 0;
					ip = scan_block(program, ip, '(', ')', '|', &found_else);
				}
			}
			break;
		case '|': ip = scan_block(program, ip, '(', ')', 0, NULL); break;
		case 'w': case 'f':
			if (program[ip + 1] == '(') {
				ip++;
				int end_ip = scan_block(program, ip, '(', ')', 0, NULL);
				if (tape[tp] == 0 && program[ip - 1] != 'f') {
					ip = end_ip;
				} else {
					loop_stack[lp++] = (Loop){ .start = ip, .end = end_ip };
				}
			}
			break;
		case 'T': switch (program[++ip]) {
			case 'i':
				initscr(), cbreak(), noecho();
				curs_set(0), keypad(stdscr, TRUE);
				ncwin = newwin(27, 82, 0, 0);
			case 'r':
				box(ncwin, 0, 0);
				mvwprintw(ncwin, 0, 6, " TSL Terminal Window ");
				for (int i = 0; i < 2000; i++) {
					unsigned char ch = tape[14000 + i] & 0xFF;
					if (ch == 0) ch = 32;
					if (ch < 32 || ch > 126) continue;
					mvwaddch(ncwin, (i / 80) + 1, (i % 80) + 1, ch);
				}
				wrefresh(ncwin);
				break;
			case 'w':
				memset(&tape[14000], 0, sizeof(tape[0]) * 2000);
				break;
			case 'f':
				delwin(ncwin), endwin();
				break;
			case 'c': tape[tp] = wgetch(ncwin); break;
			case 'C': {
				nodelay(ncwin, TRUE);
				int ch = wgetch(ncwin);
				nodelay(ncwin, FALSE);
				if (ch != ERR) {
					ungetch(ch);
					tape[tp] = 1;
				} else {
					tape[tp] = 0;
				}
				break;
			}
		} break;
	}
}

int scan_block(char* program, int ip, char open, char close, char alt, int* found_alt) {
	int balance = 1;
	char in_string = 0;

	if (found_alt) *found_alt = 0;

	while (balance > 0 && program[ip] != 0) {
		ip++;
		char ch = program[ip];

		if (in_string == 0 && (ch == '\'' || ch == '"')) in_string = ch;
		else if (in_string == ch) in_string = 0;

		if (in_string == 0) {
			if (ch == open) {
				balance++;
			} else if (ch == close) {
				balance--;
			} else if (alt != 0 && ch == alt && balance == 1) {
				if (found_alt) *found_alt = 1;
				break;
			}
		}
	}

	return ip;
}

void populate_function_list(char* program, Function* functions) {
	char in_string = 0;

	for (int i = 0; program[i] != 0; i++) {
		char ch = program[i];

		if (in_string == 0) {
			if (ch == '\'' || ch == '"') {
				in_string = ch;
			} else if (ch == '@') {
				int start_ip = i + 1, requested = 0, index = 0;
				i = scan_block(program, start_ip, '(', ')', 0, NULL);
				sscanf(&program[start_ip], "(%d,%d)", &index, &requested);
				functions[index] = (Function){.entry = i + 1, .tape_space = requested};
			}
		} else {
			if (ch == in_string) in_string = 0;
		}
	}
}

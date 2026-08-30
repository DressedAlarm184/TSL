void run_tsl_code(unsigned char* program) {
	int ip = 0, tp = 0, sp = 0, lp = 0, cp = 0;
	uint16_t user_stack[8192] = {0}, tape[32768] = {0}, variables[26] = {0};
	CallStack call_stack[512] = {0}; Loop loop_stack[64] = {0}; Function functions[1000] = {0};

	populate_program_layout(&ip, functions, program);

	for (; program[ip] != '%' && program[ip] != 0; ip++) switch (program[ip]) {
		case '+': tape[tp]++; break;
		case '-': tape[tp]--; break;
		case '>': tp++; break;
		case '<': tp--; break;
		case 'P': putchar(tape[tp]); fflush(stdout); break;
		case 'N': printf("%d", tape[tp]); fflush(stdout); break;
		case ':': {
			char sub_op = program[++ip];
			switch (sub_op) {
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
			}
			break;
		}
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
			int start_tp = tp;
			ip++;
			for (; program[ip] != '"'; ip++, tp++) {
				tape[tp] = program[ip];
			}
			tape[tp] = 0;
			tp = start_tp;
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
		case 'K': usleep(tape[tp] * 1000); break;
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
			if (entry->func->tape_space > 0) tp = entry->return_tp;
			ip = entry->return_ip;
			break;
		}
		case 'F': {
			int idx = 0; sscanf((char*)&program[ip + 1], "%3d", &idx);
			CallStack entry = (CallStack){.func = &functions[idx], .return_ip = ip + 3, .return_tp = tp};
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
	}
}

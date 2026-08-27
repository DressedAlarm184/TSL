void run_tsl_code(unsigned char* program) {
	int ip = 0, tp = 0, sp = 0, lp = 0, ap = 0, cp = 0;
	uint16_t user_stack[1024] = {0}, tape[8192] = {0}, variables[26] = {0};
	int functions[100] = {0}, call_stack[128] = {0}, addr_stack[256] = {0};
	unsigned int iterations = 0; Loop loop_stack[64] = {0};

	populate_program_layout(&ip, functions, program);

	for (; program[ip] != '%' && program[ip] != 0; ip++, iterations++) switch (program[ip]) {
		case '+': tape[tp]++; break;
		case '-': tape[tp]--; break;
		case '>': tp++; break;
		case '<': tp--; break;
		case 'P': putchar(tape[tp]); fflush(stdout); break;
		case 'N': printf("%d", tape[tp]); fflush(stdout); break;
		case 'L': tape[tp] <<= 1; break;
		case 'R': tape[tp] >>= 1; break;
		case '!': user_stack[++sp] = tape[tp]; break;
		case '$': tape[tp] = user_stack[sp--]; break;
		case '^': tape[tp] += user_stack[sp--]; break;
		case '~': tape[tp] -= user_stack[sp--]; break;
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
		case '{': {
			ip++;
			unsigned short start_addr = ip;
			char nb[8] = {0};
			while (program[ip] != '}') {
				nb[ip - start_addr] = program[ip];
				ip++;
			}
			tp = strtol(nb, NULL, 0);
			break;
		}
		case '[': {
			ip++;
			char number_buffer[8] = {0}; int buf_idx = 0; char mode = '=';
			if (program[ip] == '+' || program[ip] == '-') mode = program[ip++];
			while (program[ip] != ']') number_buffer[buf_idx++] = program[ip++];
			uint16_t val = (uint16_t)strtol(number_buffer, NULL, 0);
			if (mode == '+') {
				tape[tp] += val;
			} else if (mode == '-') {
				tape[tp] -= val;
			} else tape[tp] = val;
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
				if (tape[tp] != 0) {
					ip = loop_stack[lp - 1].start;
				} else {
					lp--;
				}
			}
			break;
		case 'X': {
			uint16_t temp = user_stack[sp];
			user_stack[sp] = tape[tp];
			tape[tp] = temp;
			break;
		}
		case 'K': usleep(tape[tp] * 1000); break;
		case '=': tape[tp] = (user_stack[sp--] == tape[tp]) ? 1 : 0; break;
		case '*': tape[tp] = (user_stack[sp--] != tape[tp]) ? 1 : 0; break;
		case 'B': tape[tp] = (user_stack[sp--] > tape[tp]) ? 1 : 0; break;
		case 'S': tape[tp] = (user_stack[sp--] < tape[tp]) ? 1 : 0; break;
		case '\'':
			ip++;
			while (program[ip] != '\'') {
				if (program[ip] == '\\') switch (program[++ip]) {
					case 'n': putchar('\n'); break;
					case 'r': putchar('\r'); break;
					case 't': putchar('\t'); break;
					default: putchar('?'); break;
				} else if (program[ip] == '%') switch (program[++ip]) {
					case '%': putchar('%'); break;
					case 's': printf("%s", &tape[tp]); break;
					case 'd': printf("%d", tape[tp]); break;
					case 'c': printf("%c", tape[tp]); break;
					default: putchar('?'); break;
				} else {
					putchar(program[ip]);
				}
				ip++;
			}
			fflush(stdout);
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
		case 'Q': ip = call_stack[--cp]; break;
		case 'F': {
			int func_index = 0;
			sscanf((char*)&program[ip + 1], "%2d", &func_index);
			call_stack[cp++] = ip + 2;
			ip = functions[func_index] - 1;
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
		case 'w':
			if (program[ip + 1] == '(') {
				ip++;
				int end_ip = scan_block(program, ip, '(', ')', 0, NULL);
				if (tape[tp] == 0) {
					ip = end_ip;
				} else {
					loop_stack[lp++] = (Loop){ .start = ip, .end = end_ip };
				}
			}
			break;
	}
}

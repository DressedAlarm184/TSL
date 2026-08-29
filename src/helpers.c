int scan_block(const unsigned char* program, int ip, char open, char close, char alt, int* found_alt) {
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

void populate_program_layout(int* ip, Function* functions, unsigned char* program) {
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
			} else if (ch == 'S' && *ip == 0) {
				*ip = i + 1;
			}
		} else {
			if (ch == in_string) in_string = 0;
		}
	}
}

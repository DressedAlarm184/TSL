#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

int run_tsl_code(char* program);

int main(int argc, char* argv[]) {
	srand(time(NULL));
	
	char* input;

	printf("TSLv3 Shell\n");

	while ((input = readline("tslsh> ")) != NULL) {
		if (strlen(input) > 0) {
			add_history(input);
			run_tsl_code(input);
		}

		free(input);
	}
}

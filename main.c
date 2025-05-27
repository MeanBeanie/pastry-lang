#include <stdio.h>
#include <string.h>
#include "interpreter.h"

int main(int argc, char** argv){
	if(argc > 3 || argc < 2){
		printf("Usage: frosting [file]\nNOTE: Run without args to enter live mode\n");
	}
	else if(argc == 2 || argc == 3){
		FILE* file = fopen(argv[1], "r");
		if(file == NULL){
			fprintf(stderr, "File at %s does not exit\n", argv[1]);
			return 1;
		}
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		fseek(file, 0, SEEK_SET);
		char buffer[size+1];
		fread(buffer, sizeof(char), size, file);
		buffer[size] = '\0';
		return run_code(buffer, size, (argc == 2 ? 1 : strncmp(argv[2], "debug", 5)));
	}
	return 0;
}

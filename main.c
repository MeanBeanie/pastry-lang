#include <stdio.h>
#include "interpreter.h"

int main(int argc, char** argv){
	if(argc > 2){
		printf("Usage: frosting [file]\nNOTE: Run without args to enter live mode\n");
	}
	else if(argc == 2){
		FILE* file = fopen(argv[1], "r");
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		fseek(file, 0, SEEK_SET);
		char buffer[size+1];
		fread(buffer, sizeof(char), size, file);
		buffer[size] = '\0';
		return run_code(buffer, size);
	}
	else{
		for(;;){
			char buffer[64] = {0};
			fgets(buffer, 64, stdin);
			if(buffer[63] != '\0'){
				buffer[63] = '\0';
			}
			int err = run_code(buffer, 64);
			if(err != 0){
				fprintf(stderr, "Code exited with failure code %i\n", err);
				break; // code exited with a failure
			}
		}
	}
	return 0;
}

#ifndef INTERPRETER_H
#define INTERPRETER_H
#include <stddef.h>
#include "lexer.h"

typedef struct {
	char* name;
	size_t name_size;

	enum TokenType type;
	char* str;
	size_t str_size;
} Var;

int run_code(char* src, size_t size, int debug_mode);

#endif // INTERPRETER_H

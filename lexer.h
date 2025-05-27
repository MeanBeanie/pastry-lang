#ifndef LEXER_H
#define LEXER_H
#include <stddef.h>

#define ERROR_LOG(res,...) do { res.exit_code = 1; fprintf(stderr, __VA_ARGS__); } while(0)

enum TokenType {
	GROUP_START, GROUP_END, // 1
	EQEQ, LT, LTEQ, GT, GTEQ, // 6
	PLUS, MINUS, STAR, SLASH, COMMA, // 11

	IDENTIFIER, INTEGER, STRING, // 14

	VAR, PRINT, READ, // 17
	FOR, WHILE, // 19
	IF, ELSE, ELIF, // 22
	AND, OR, NOT, // 25
	EXIT, END, // 27
	FUNC, CALL, // 29

	NEWLINE // 30
};

typedef struct {
	enum TokenType type;
	char* str;
	size_t size;
} Token;

typedef struct {
	Token* tokens;
	size_t size;
	size_t capacity;
	int exit_code;
} Lexer;

Lexer lex(char* src, size_t size);
void print_lexer(Lexer lexer);
void free_lexer(Lexer* lexer);

#endif // LEXER_H

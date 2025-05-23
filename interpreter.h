#ifndef INTERPRETER_H
#define INTERPRETER_H
#include <stddef.h>
#define DEBUG

#define ADD_TOKEN(lexer,t,off,s) \
	if(lexer.size >= lexer.capacity){\
		lexer.capacity *= 2;\
		lexer.tokens = realloc(lexer.tokens, lexer.capacity*sizeof(Token));\
	}\
	lexer.tokens[lexer.size] = (Token){.type = t, .offset = off, .size = s};\
	lexer.size++

#define IS_RESERVED(string,compare,token) if(strncmp(string, compare, strlen(compare)) == 0){ return token; }

enum Error {
	ERROR_NONE = 0,
	ERROR_UNKNOWN,
	ERROR_WRONG_ARGS,
	ERROR_EXTRA,
	ERROR_DNE,
	ERROR_WRONG_TYPE,
};

enum TokenType {
	OPEN_PAREN, CLOSE_PAREN, OPEN_CURLY, CLOSE_CURLY, // 3
	EQ, EQEQ, LT, LEQ, GT, GEQ, // 9
	PLUS, MINUS, STAR, SLASH, DOT, COMMA, // 15
	// LITERAL
	IDENTIFIER, STRING, NUMBER, // 18
	// RESERVE
	VAR, PRINT, READ, // 21
	FOR, WHILE, // 23
	IF, ELSE, // 25
	AND, OR, NOT, // 28
	EXIT, END, // 30
	FUNC, CALL, // 32
	// MISC
	NEWLINE, // 33
};

typedef struct {
	enum TokenType type;
	int offset; // character offset from the start of the src
	size_t size; // how long after the offset does the token take
} Token;

// basically just a dynamic array of tokens
typedef struct {
	Token* tokens;
	size_t size;
	size_t capacity;
	int exit;
} Lexer;

enum ExprType {
	LITERAL = 0,
	GROUP,
	BOOL,
	MATH,
	VAR_SET = 4,
	VAR_REASSIGN,
	VAR_REF,
	FUN_CALL = 7
};

typedef struct Expr_Group Expr_Group;
typedef struct Expr_TokenOp Expr_TokenOp;
typedef struct Expr_Var_Set Expr_Var_Set;
typedef struct Expr_Fun_Call Expr_Fun_Call;

union Expr_As{
	Token* literal;
	Expr_Group* group;
	Expr_TokenOp* token_op;
	Expr_Var_Set* var_set;
	Expr_Fun_Call* fun_call;
};

typedef struct {
	enum ExprType type;
	union Expr_As as;
} Expr;

typedef struct Expr_Group {
	enum ExprType internal;
	Expr as;
} Expr_Group;
// collapses to a bool when processed if boolean
// otherwise collapses to the solution to the math op
typedef struct Expr_TokenOp {
	Expr lhs;
	Token operator;
	Expr rhs;
} Expr_TokenOp;
typedef struct Expr_Var_Set {
	Token name;
	Expr value;
} Expr_Var_Set;
typedef struct Expr_Fun_Call {
	Token name;
	size_t argc;
	Expr* args; // args can be expressions
} Expr_Fun_Call;

typedef struct {
	size_t offset;
	size_t size;
	Expr* exprs;
	size_t argc;
	Token* args; // just the identifiers with the names
} Function;

typedef struct {
	Expr* expressions;
	size_t size;
	size_t capacity;
	int exit;
	Function* functions;
	size_t function_count;
} Parser;

typedef struct {
	int n_offset;
	size_t n_size;
	enum TokenType type;
	int integer;
	int s_offset;
	size_t s_size;
} Var;

int run_code(char* src, int size);

#endif // INTERPRETER_H

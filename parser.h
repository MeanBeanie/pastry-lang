#ifndef PARSER_H
#define PARSER_H
#include <stddef.h>
#include "lexer.h"

enum ExprType {
	LITERAL, // also includes var identifiers
	GROUPED,
	OPERATION,
	FUNCTION_CALL, // variables are just calling a `var` function
};

struct Expr_Group;
struct Expr_Op;
struct Expr_Function_Call;
struct Expr_Var_Set;

union ExprAs {
	Token* literal;
	struct Expr_Group* grouped;
	struct Expr_Op* operation;
	struct Expr_Function_Call* function_call;
};

typedef struct {
	enum ExprType type;
	union ExprAs as;
} Expr;

struct Expr_Group {
	Expr expr;
};
struct Expr_Op {
	Expr lhs;
	enum TokenType operator;
	Expr rhs;
};
struct Expr_Function_Call {
	enum TokenType type;
	Expr* argv;
	size_t argc;
};

typedef struct {
	char* name;
	size_t name_size;
	Expr* exprs;
	size_t size;
	size_t capacity;
	Token* argv;
	size_t argc;
	size_t arg_capacity;
} Function;

typedef struct {
	Expr* exprs;
	size_t size;
	size_t capacity;
	Function* functions;
	size_t function_count;
	size_t function_capacity;
	int exit_code;
} Parser;

Parser parse(Lexer lexer);
void print_parser(Parser parser);
void free_parser(Parser* parser);

#endif // PARSER_H

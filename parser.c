#include "parser.h"
#include "lexer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

void add_expression(Expr** exprs, size_t* size, size_t* capacity, Expr expr){
	if(capacity != NULL){
		if(*size >= *capacity){
			*capacity *= 2;
			*exprs = realloc((*exprs), (*capacity)*sizeof(Expr));
		}
	}
	else{
		if(*size >= 8){
			fprintf(stderr, "[ERR] Out of bounds for fixed size expr array\n");
			return;
		}
	}
	(*exprs)[*size] = expr;
	*size = (*size) + 1;
}

Parser parse(Lexer lexer){
	Parser res = {
		.size = 0,
		.capacity = 8,
		.exprs = malloc(8*sizeof(Expr)),
		.function_count = 0,
		.function_capacity = 8,
		.functions = malloc(8*sizeof(Function)),
		.exit_code = 0,
	};

	int inFunction = 0;
	int skipEnd = 0;
	int savingRHS = 0;
	int inFunctionCall = 0;

	for(size_t i = 0; i < lexer.size; i++){
		Token token = lexer.tokens[i];
		Expr* exprs = NULL;
		size_t* size = NULL;
		size_t* capacity = NULL;
		if(inFunction == 0){
			exprs = res.exprs;
			size = &res.size;
			capacity = &res.capacity;
		}
		else{
			exprs = res.functions[res.function_count].exprs;
			size = &res.functions[res.function_count].size;
			capacity = &res.functions[res.function_count].capacity;
		}
		if(exprs == NULL || size == NULL || capacity == NULL){
			ERROR_LOG(res, "[ERR] Failed to get the expression list details\n");
			break;
		}
		if(inFunctionCall == 1){
			size_t t = (*size)-1;
			size = &exprs[t].as.function_call->argc;
			exprs = exprs[t].as.function_call->argv;
			capacity = NULL;
		}
		switch(token.type){
			case NEWLINE:
			{
				if(inFunctionCall == 1){
					inFunctionCall = 0;
				}
				break;
			}
			case INTEGER:
			case STRING:
			{
				if((i >= 1 && (lexer.tokens[i-1].type >= GROUP_START && lexer.tokens[i-1].type <= COMMA))
				|| (i+1 < lexer.size && (lexer.tokens[i+1].type >= GROUP_START && lexer.tokens[i+1].type <= COMMA))){
					break;
				}
				Expr expr = {0};
				expr.type = LITERAL;
				expr.as.literal = &lexer.tokens[i];
				add_expression(&exprs, size, capacity, expr);
				break;
			}
			case IDENTIFIER:
			{
				if((i >= 1 && (lexer.tokens[i-1].type >= GROUP_START && lexer.tokens[i-1].type <= COMMA))
				|| (i+1 < lexer.size && (lexer.tokens[i+1].type >= GROUP_START && lexer.tokens[i+1].type <= COMMA))){
					break;
				}
				Expr expr = {0};
				expr.type = LITERAL;
				expr.as.literal = &lexer.tokens[i];
				add_expression(&exprs, size, capacity, expr);
				break;
			}
			case EQEQ: case LTEQ: case GTEQ: case LT: case GT:
			case PLUS: case MINUS: case STAR: case SLASH:
			{
				Expr expr = {0};
				expr.type = OPERATION;
				expr.as.operation = malloc(sizeof(struct Expr_Op));
				expr.as.operation->operator = token.type;

				if(i == 0 || i+1 >= lexer.size){
					ERROR_LOG(res, "[ERR] Operation expression cannot be the first or last token\n");
					break;
				}

				// LHS
				if(lexer.tokens[i-1].type == IDENTIFIER
				|| lexer.tokens[i-1].type == INTEGER
				|| lexer.tokens[i-1].type == STRING){
					expr.as.operation->lhs.type = LITERAL;
					expr.as.operation->lhs.as.literal = &lexer.tokens[i-1];
				}
				else if(lexer.tokens[i-1].type == GROUP_END){
					expr.as.operation->lhs.type = GROUPED;
					expr.as.operation->lhs.as.grouped = malloc(sizeof(struct Expr_Group));
					expr.as.operation->lhs.as.grouped->expr = exprs[(*size)-1].as.grouped->expr;
					*size = (*size) - 1;
				}
				else{
					ERROR_LOG(res, "[ERR] Operation expression does not support token type %i\n", lexer.tokens[i-1].type);
					break;
				}

				// RHS
				if(lexer.tokens[i+1].type == IDENTIFIER
				|| lexer.tokens[i+1].type == INTEGER
				|| lexer.tokens[i+1].type == STRING){
					expr.as.operation->rhs.type = LITERAL;
					expr.as.operation->rhs.as.literal = &lexer.tokens[i+1];
					i++;
				}
				else if(lexer.tokens[i+1].type == GROUP_START){
					savingRHS = 1;
				}
				else{
					ERROR_LOG(res, "[ERR] Operation expression does not support token type %i\n", lexer.tokens[i+1].type);
					break;
				}

				add_expression(&exprs, size, capacity, expr);

				break;
			}
			case GROUP_END:
			{
				Expr expr = {0};
				expr.type = GROUPED;
				if((*size) == 0){
					ERROR_LOG(res, "[ERR] Group expression requires something inside of it\n");
					break;
				}
				expr.as.grouped = malloc(sizeof(struct Expr_Group));
				expr.as.grouped->expr = exprs[(*size)-1];
				if(savingRHS == 1){
					exprs[(*size)-2].as.operation->rhs = expr;
					*size = (*size)-1;
					savingRHS = 0;
					break;
				}
				exprs[(*size)-1] = expr;
				break;
			}
			default:
			{
				if(token.type >= VAR && token.type <= CALL){
					if(token.type == FUNC){
						Function function = {
							.name = NULL,
							.size = 0,
							.capacity = 8,
							.exprs = malloc(8*sizeof(Expr)),
							.argc = 0,
							.arg_capacity = 8,
							.argv = malloc(8*sizeof(Token)),
						};
						if(i+1 < lexer.size && lexer.tokens[i+1].type == IDENTIFIER){
							function.name_size = lexer.tokens[i+1].size;
							function.name = malloc((function.name_size+1)*sizeof(char));
							strncpy(function.name, lexer.tokens[i+1].str, lexer.tokens[i+1].size);
							function.name[function.name_size] = '\0';
						}
						else{
							ERROR_LOG(res, "Function definitions require a name after the func keyword\n");
							break;
						}
						i += 2;
						while(i < lexer.size && lexer.tokens[i].type != NEWLINE){
							if(function.argc >= function.arg_capacity){
								function.arg_capacity *= 2;
								function.argv = realloc(function.argv, function.arg_capacity*sizeof(Token));
							}
							function.argv[function.argc] = lexer.tokens[i];
							function.argc++;
							i++;
						}
						if(i >= lexer.size){
							ERROR_LOG(res, "[ERR] Unbounded arguments in function declaration for %.*s\n", (int)function.name_size, function.name);
							break;
						}
						res.functions[res.function_count] = function;
						inFunction = 1;
						break;
					}
					if(token.type == END){
						if(inFunction == 1 && skipEnd == 0){
							res.function_count++;
							if(res.function_count >= res.function_capacity){
								res.function_capacity *= 2;
								res.functions = realloc(res.functions, res.function_capacity*sizeof(Function));
							}
							inFunction = 0;
							break;
						}
						skipEnd = 0;
					}
					if(token.type == IF || token.type == FOR || token.type == WHILE){
						skipEnd = 1;
					}
					Expr expr = {0};
					expr.type = FUNCTION_CALL;
					expr.as.function_call = malloc(sizeof(struct Expr_Function_Call));

					expr.as.function_call->type = token.type;
					expr.as.function_call->argc = 0;
					expr.as.function_call->argv = malloc(sizeof(Expr)*8);
					inFunctionCall = 1;

					add_expression(&exprs, size, capacity, expr);
				}
				break;
			}
		};
	}

	return res;
}

void print_expression(int indents, Expr expr){
	char str[indents+1];
	for(int i = 0; i < indents; i++){
		str[i] = ' ';
	}
	str[indents] = '\0';
	switch(expr.type){
		case OPERATION:
		{
			print_expression(indents, expr.as.operation->lhs);
			printf("%sOperation: %i\n", str, expr.as.operation->operator);
			print_expression(indents, expr.as.operation->rhs);
			break;
		}
		case LITERAL:
		{
			printf("%sLiteral: %.*s\n", str, (int)expr.as.literal->size, expr.as.literal->str);
			break;
		}
		case GROUPED:
		{
			printf("%s(\n", str);
			print_expression(indents+1, expr.as.grouped->expr);
			printf("%s)\n", str);
			break;
		}
		case FUNCTION_CALL:
		{
			printf("%s%i w/ args:{\n", str, expr.as.function_call->type);
			for(size_t i = 0; i < expr.as.function_call->argc; i++){
				print_expression(indents+1, expr.as.function_call->argv[i]);
			}
			printf("%s}\n", str);
		}
		default: break;
	}
}

void print_parser(Parser parser){
	if(parser.exprs == NULL && parser.functions == NULL){
		printf("[DEBG] Parser is empty\n");
		return;
	}
	printf("--Parser--\n");
	printf("Global\n");
	for(size_t i = 0; i < parser.size; i++){
		printf("Expr %i (type %i): [\n", (int)i, parser.exprs[i].type);
		print_expression(1, parser.exprs[i]);
		printf("]\n");
	}
	for(size_t i = 0; i < parser.function_count; i++){
		printf("(func %.*s) w/ args:{ ", (int)parser.functions[i].name_size, parser.functions[i].name);
		for(size_t j = 0; j < parser.functions[i].argc; j++){
			printf("%.*s ", (int)parser.functions[i].argv[j].size, parser.functions[i].argv[j].str);
		}
		printf("}\n");
		for(size_t j = 0; j < parser.functions[i].size; j++){
			printf("Expr %i (type %i): [\n", (int)j, parser.functions[i].exprs[j].type);
			print_expression(1, parser.functions[i].exprs[j]);
			printf("]\n");
		}
	}
}

void free_expression(Expr* expr){
	switch(expr->type){
		case LITERAL:
		{
			// the token gets freed by the lexer
			expr->as.literal = NULL;
			break;
		}
		case OPERATION:
		{
			free_expression(&expr->as.operation->lhs);
			free_expression(&expr->as.operation->rhs);
			free(expr->as.operation);
			break;
		}
		case GROUPED:
		{
			free_expression(&expr->as.grouped->expr);
			free(expr->as.grouped);
			break;
		}
		case FUNCTION_CALL:
		{
			for(size_t i = 0; i < expr->as.function_call->argc; i++){
				free_expression(&expr->as.function_call->argv[i]);
			}
			free(expr->as.function_call->argv);
		}
		default: break;
	}
}

void free_parser(Parser* parser){
	for(size_t i = 0; i < parser->size; i++){
		free_expression(&parser->exprs[i]);
	}
	for(size_t i = 0; i < parser->function_count; i++){
		free(parser->functions[i].name);
		for(size_t j = 0; j < parser->functions[i].size; j++){
			free_expression(&parser->functions[i].exprs[j]);
		}
	}
	free(parser->functions);
	parser->functions = NULL;
	free(parser->exprs);
	parser->exprs = NULL;
}

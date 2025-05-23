#include "interpreter.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

enum TokenType identifier(char* str){
	IS_RESERVED(str, "var", VAR);
	IS_RESERVED(str, "print", PRINT);
	IS_RESERVED(str, "read", READ);
	IS_RESERVED(str, "for", FOR);
	IS_RESERVED(str, "while", WHILE);
	IS_RESERVED(str, "if", IF);
	IS_RESERVED(str, "else", ELSE);
	IS_RESERVED(str, "and", AND);
	IS_RESERVED(str, "or", OR);
	IS_RESERVED(str, "not", NOT);
	IS_RESERVED(str, "exit", EXIT);
	IS_RESERVED(str, "end", END);
	IS_RESERVED(str, "func", FUNC);
	IS_RESERVED(str, "call", CALL);
	return IDENTIFIER;
}

void print_expression(char* src, const char* prepend, Expr expr){
	printf("%s", prepend);
	switch(expr.type){
		case BOOL:
		case MATH:
		{
			printf("Type: %i[\n", expr.type);
			print_expression(src, "\t", expr.as.token_op->lhs);
			print_expression(src, "\t", expr.as.token_op->rhs);
			printf("]\n");
			break;
		}
		case GROUP:
		{
			printf("Type: %i(\n", expr.type);
			print_expression(src, "\t", expr.as.group->as);
			printf(")\n");
			break;
		}
		case LITERAL:
		{
			printf("Type: %i, Value: %.*s\n", expr.type, (int)expr.as.literal->size, src+expr.as.literal->offset);
			break;
		}
		case VAR_REF:
		{
			printf("Type: %i, Name: %.*s\n", expr.type, (int)expr.as.literal->size, src+expr.as.literal->offset);
			break;
		}
		case VAR_REASSIGN:
		case VAR_SET:
		{
			printf("Type: %i\n", expr.type);
			printf("\tName: %.*s\n", (int)expr.as.var_set->name.size, src+expr.as.var_set->name.offset);
			print_expression(src, "\t", expr.as.var_set->value);
			break;
		}
		case FUN_CALL:
		{
			printf("Type: %i\n", expr.type);
			printf("\tName: %.*s\n", (int)expr.as.fun_call->name.size, src+expr.as.fun_call->name.offset);
			for(int i = 0; i < (int)expr.as.fun_call->argc; i++){
				print_expression(src, "\t", expr.as.fun_call->args[i]);
			}
			break;
		}
		default:
		{
			printf("default: Type: %i\n", expr.type);
			break;
		}
	}
}

char* preprocess(char* src, int* size){
	int og_size = (*size);
	char* res = NULL;
	res = realloc(res, strlen(src)+1);
	strncpy(res, src, (*size));
	res[(*size)] = '\0';
	int instruction = 0;
	int instruction_size = 0;
	for(int i = 0; i < og_size; i++){
		if(instruction == 1){
			if(src[i] == '\n'){
				char filepath[instruction_size+1];
				strncpy(filepath, src+i-instruction_size, instruction_size);
				filepath[instruction_size] = '\0';

				FILE* file = fopen(filepath, "r");
				if(file == NULL){
					fprintf(stderr, "preprocess: Cannot find file at path \"%s\"\n", filepath);
					return NULL;
				}
				fseek(file, 0, SEEK_END);
				int f_size = ftell(file);
				fseek(file, 0, SEEK_SET);
				char buffer[f_size+1];
				fread(buffer, sizeof(char), f_size, file);
				buffer[f_size] = '\0';

				char* temp = malloc(strlen(res)+strlen(buffer)+1);
				for(int j = 0; j < (*size); j++){
					temp[j+strlen(buffer)] = res[j];
				}
				*size = strlen(res) + strlen(buffer);
				strncpy(temp, buffer, strlen(buffer));
				temp[(*size)] = '\0';
				free(res);
				res = temp;

				instruction_size = 0;
				instruction = 0;
			}
			else{
				instruction_size++;
			}
		}
		if(src[i] == '!'){
			instruction = 1;
		}
	}
	return res;
}

Lexer lex(char* src, int size){
	Lexer res = {
		.size = 0,
		.capacity = 8,
		.tokens = malloc(8*sizeof(Token)),
		.exit = ERROR_NONE,
	};

	int inSomething = 0;
	int str_size = 0;
	for(int i = 0; i < size; i++){
		char c = src[i];
		if(inSomething == 1){ // in comment
			if(c == '\n'){
				inSomething = 0;
			}
			continue;
		}
		else if(inSomething == 2){ // in string
			if(c == '"'){
				inSomething = 0;
				ADD_TOKEN(res, STRING, i-str_size, str_size);
				continue;
			}
			str_size++;
			continue;
		}
		else if(inSomething == 3){ // in number
			if(!isdigit(c)){
				inSomething = 0;
				ADD_TOKEN(res, NUMBER, i-str_size-1, str_size+1);
			}
			else{
				str_size++;
				continue;
			}
		}
		else if(inSomething == 4){ // in identifier
			if(!isalnum(c)){
				inSomething = 0;
				enum TokenType type = identifier(src+i-str_size-1);
				ADD_TOKEN(res, type, i-str_size-1, str_size+1);
			}
			else{
				str_size++;
				continue;
			}
		}
		switch(c){
			case '\0':
			{
				i = size;
				break;
			}
			case '\n':
			{
				ADD_TOKEN(res, NEWLINE, i, 1);
				break;
			}
			case ' ':
			case '\t':
			case '\r':
			{
				break;
			}
			case '!':
			{
				inSomething = 1;
				break;
			}
			case '(': ADD_TOKEN(res, OPEN_PAREN, i, 1); break;
			case ')': ADD_TOKEN(res, CLOSE_PAREN, i, 1); break;
			case '{': ADD_TOKEN(res, OPEN_CURLY, i, 1); break;
			case '}': ADD_TOKEN(res, CLOSE_CURLY, i, 1); break;
			case '+': ADD_TOKEN(res, PLUS, i, 1); break;
			case '-': ADD_TOKEN(res, MINUS, i, 1); break;
			case '*': ADD_TOKEN(res, STAR, i, 1); break;
			case '.': ADD_TOKEN(res, DOT, i, 1); break;
			case ',': ADD_TOKEN(res, COMMA, i, 1); break;
			case '/':
			{
				if(i+1 < size && src[i+1] == '/'){
					inSomething = 1;
					break;
				}
				ADD_TOKEN(res, SLASH, i, 1);
				break;
			}
			case '=':
			{
				if(i+1 < size && src[i+1] == '='){
					ADD_TOKEN(res, EQEQ, i, 2);
					i++;
					break;
				}
				ADD_TOKEN(res, EQ, i, 1);
				break;
			}
			case '>':
			{
				if(i+1 < size && src[i+1] == '='){
					ADD_TOKEN(res, GEQ, i, 2);
					i++;
					break;
				}
				ADD_TOKEN(res, GT, i, 1);
				break;
			}
			case '<':
			{
				if(i+1 < size && src[i+1] == '='){
					ADD_TOKEN(res, LEQ, i, 2);
					i++;
					break;
				}
				ADD_TOKEN(res, LT, i, 1);
				break;
			}
			case '"':
			{
				str_size = 0;
				inSomething = 2;
				break;
			}
			default:
			{
				if(isdigit(c)){
					inSomething = 3;
					str_size = 0;
					break;
				}
				else if(isalpha(c)){
					inSomething = 4;
					str_size = 0;
					break;
				}
				fprintf(stderr, "Unknown character: \'%c\'\n", c);
				res.exit = ERROR_UNKNOWN;
				break;
			}
		}
	}

	return res;
}

Parser parse(Lexer lexer){
	Parser res = {
		.size = 0,
		.capacity = 8,
		.expressions = malloc(8*sizeof(Expr)),
		.exit = ERROR_NONE,
		.function_count = 0,
		.functions = malloc(8*sizeof(Function)),
	};

	int line = 1;
	int inGroup = 0;
	int savingRHS = 0;
	int savingVar = 0;
	int savingVarReassign = 0;
	int inFunctionCall = 0;
	int inFunctionBody = 0;
	for(int i = 0; i < (int)lexer.size; i++){
		Token token = lexer.tokens[i];
		if(inFunctionCall == 1 && token.type != NEWLINE && (token.type > NUMBER || token.type < IDENTIFIER)){
			fprintf(stderr, "Cannot have anything but a variable or a literal in a function arg\n");
			res.exit = ERROR_WRONG_ARGS;
			continue;
		}
		switch(token.type){
			case NEWLINE:
			{
				if(inFunctionCall == 1){
					inFunctionCall = 0;
				}
				line++;
				break;
			}
			case IDENTIFIER:
			{
				if(savingRHS == 1){
					res.expressions[res.size-1].as.token_op->rhs.type = VAR_REF;
					res.expressions[res.size-1].as.token_op->rhs.as.literal = &lexer.tokens[i];
					savingRHS = 0;
				}
				else if(savingVar == 1){
					res.expressions[res.size-1].as.var_set->value.type = VAR_REF;
					res.expressions[res.size-1].as.var_set->value.as.literal = &lexer.tokens[i];
					savingVar = 0;
				}
				else if(inFunctionCall == 1){	
					Expr expr = {0};
					expr.type = VAR_REF;
					expr.as.literal = &lexer.tokens[i];
					res.expressions[res.size-1].as.fun_call->args[res.expressions[res.size-1].as.fun_call->argc] = expr;
					res.expressions[res.size-1].as.fun_call->argc++;
					if(res.expressions[res.size-1].as.fun_call->argc >= 16){
						fprintf(stderr, "Too many args in function\n");
						res.exit = ERROR_WRONG_ARGS;
						inFunctionCall = 0;
					}
				}
				else if(i+1 < (int)lexer.size && lexer.tokens[i+1].type == EQ){
					// actually a var reassign
					if(i+2 >= (int)lexer.size){
						fprintf(stderr, "LINE %i: Variable reassignment has wrong number of args\n", line);
						res.exit = ERROR_WRONG_ARGS;
						break;
					}
					if(res.size >= res.capacity){
						res.capacity *= 2;
						res.expressions = realloc(res.expressions, res.capacity*sizeof(Token));
					}
					res.expressions[res.size].type = VAR_REASSIGN;
					res.expressions[res.size].as.var_set = malloc(sizeof(Expr_Var_Set));
					if(lexer.tokens[i].type != IDENTIFIER){
						fprintf(stderr, "LINE %i: Variable name is not a valid name\n", line);
						res.exit = ERROR_WRONG_ARGS;
						break;
					}
					res.expressions[res.size].as.var_set->name = lexer.tokens[i];
					if(lexer.tokens[i+2].type == OPEN_PAREN){
						savingVarReassign = 1;
						i++;
					}
					else if(lexer.tokens[i+2].type == IDENTIFIER){
						res.expressions[res.size].as.var_set->value.type = VAR_REF;
						res.expressions[res.size].as.var_set->value.as.literal = &lexer.tokens[i+2];
						i += 2;
					}
					else{
						res.expressions[res.size].as.var_set->value.type = LITERAL;
						res.expressions[res.size].as.var_set->value.as.literal = &lexer.tokens[i+2];
						i += 2;
					}
					res.size++;
				}
				else{
					if(res.size >= res.capacity){
						res.capacity *= 2;
						res.expressions = realloc(res.expressions, res.capacity*sizeof(Token));
					}
					res.expressions[res.size].type = VAR_REF;
					res.expressions[res.size].as.literal = &lexer.tokens[i];
					res.size++;
				}
				break;
			}
			case LEQ:
			case GEQ:
			case LT:
			case GT:
			case EQEQ:
			case PLUS:
			case MINUS:
			case STAR:
			case SLASH:
			{
				if((i-1 < 0 || i+1 >= (int)lexer.size) || (lexer.tokens[i+1].type == NEWLINE)){
					fprintf(stderr, "LINE %i: Token operation has no lhs/rhs\n", line);
					res.exit = ERROR_WRONG_ARGS;
					break;
				}
				enum ExprType type = (token.type <= 9 ? BOOL : MATH);
				if(res.size >= res.capacity){
					res.capacity *= 2;
					res.expressions = realloc(res.expressions, res.capacity*sizeof(Token));
				}
				Expr expr = {0};
				expr.type = type;
				expr.as.token_op = malloc(sizeof(Expr_TokenOp));
				if(res.size > 0 &&
					(res.expressions[res.size-1].type == GROUP || res.expressions[res.size-1].type == VAR_REF)){
					expr.as.token_op->lhs = res.expressions[res.size-1];
					res.size--;
				}
				else{
					expr.as.token_op->lhs.type = LITERAL;
					expr.as.token_op->lhs.as.literal = &lexer.tokens[i-1];
				}
				expr.as.token_op->operator = lexer.tokens[i];
				if(i+1 < (int)lexer.size &&
					(lexer.tokens[i+1].type == OPEN_PAREN || lexer.tokens[i+1].type == IDENTIFIER)){
					savingRHS = 1;
				}
				else{
					expr.as.token_op->rhs.type = LITERAL;
					expr.as.token_op->rhs.as.literal = &lexer.tokens[i+1];
					i++;
				}
				res.expressions[res.size] = expr;
				res.size++;
				break;
			}
			case OPEN_PAREN:
			{
				inGroup++;
				break;
			}
			case CLOSE_PAREN:
			{
				if(inGroup <= 0){
					fprintf(stderr, "LINE %i: Closed unopened parenthesis\n", line);
					res.exit = ERROR_EXTRA;
					break;
				}
				if(savingRHS == 1){
					res.size--;
					res.expressions[res.size-1].as.token_op->rhs.type = GROUP;
					res.expressions[res.size-1].as.token_op->rhs.as.group = malloc(sizeof(Expr_Group));
					res.expressions[res.size-1].as.token_op->rhs.as.group->internal = res.expressions[res.size].type;
					res.expressions[res.size-1].as.token_op->rhs.as.group->as = res.expressions[res.size];
					savingRHS = 0;
				}
				else if(savingVar == 1){
					res.size--;
					res.expressions[res.size-1].as.var_set->value.type = GROUP;
					res.expressions[res.size-1].as.var_set->value.as.group = malloc(sizeof(Expr_Group));
					res.expressions[res.size-1].as.var_set->value.as.group->internal = res.expressions[res.size].type;
					res.expressions[res.size-1].as.var_set->value.as.group->as = res.expressions[res.size];
					savingVar = 0;
				}
				else if(savingVarReassign == 1){
					res.size--;
					res.expressions[res.size-1].as.var_set->value.type = GROUP;
					res.expressions[res.size-1].as.var_set->value.as.group = malloc(sizeof(Expr_Group));
					res.expressions[res.size-1].as.var_set->value.as.group->internal = res.expressions[res.size].type;
					res.expressions[res.size-1].as.var_set->value.as.group->as = res.expressions[res.size];
					savingVarReassign = 0;
				}
				else{
					Expr expr = {0};
					res.size--;
					expr.type = GROUP;
					expr.as.group = malloc(sizeof(Expr_Group));
					expr.as.group->internal = res.expressions[res.size].type;
					expr.as.group->as = res.expressions[res.size];
					res.expressions[res.size] = expr;
					res.size++;
				}
				inGroup--;
				break;
			}
			case VAR:
			{
				if(i+3 >= (int)lexer.size){
					fprintf(stderr, "LINE %i: Variable assignment has wrong number of args\n", line);
					res.exit = ERROR_WRONG_ARGS;
					break;
				}
				else if(lexer.tokens[i+2].type != EQ){
					fprintf(stderr, "LINE %i: Variable assignment lacks assignment operator\n", line);
					res.exit = ERROR_WRONG_ARGS;
					break;
				}
				if(res.size >= res.capacity){
					res.capacity *= 2;
					res.expressions = realloc(res.expressions, res.capacity*sizeof(Token));
				}
				res.expressions[res.size].type = VAR_SET;
				res.expressions[res.size].as.var_set = malloc(sizeof(Expr_Var_Set));
				if(lexer.tokens[i+1].type != IDENTIFIER){
					fprintf(stderr, "LINE %i: Variable name is not a valid name\n", line);
					res.exit = ERROR_WRONG_ARGS;
					break;
				}
				res.expressions[res.size].as.var_set->name = lexer.tokens[i+1];
				if(lexer.tokens[i+3].type == OPEN_PAREN
				|| lexer.tokens[i+3].type == IDENTIFIER){
					savingVar = 1;
					i += 2;
				}
				else{
					res.expressions[res.size].as.var_set->value.type = LITERAL;
					res.expressions[res.size].as.var_set->value.as.literal = &lexer.tokens[i+3];
					i += 3;
				}
				res.size++;
				break;
			}
			default:
			{
				if(inFunctionCall == 1 && (token.type == NUMBER || token.type == STRING)){
					Expr expr = {0};
					expr.type = LITERAL;
					expr.as.literal = &lexer.tokens[i];
					res.expressions[res.size-1].as.fun_call->args[res.expressions[res.size-1].as.fun_call->argc] = expr;
					res.expressions[res.size-1].as.fun_call->argc++;
					if(res.expressions[res.size-1].as.fun_call->argc >= 16){
						fprintf(stderr, "Too many args in function\n");
						res.exit = ERROR_WRONG_ARGS;
						inFunctionCall = 0;
					}
				}
				if(token.type >= VAR && token.type <= CALL){
					if(inFunctionBody == 1 && token.type == END){
						inFunctionBody = 0;
						break;
					}
					if(token.type == FUNC){
						inFunctionBody = 1;
						if(i+1 >= (int)lexer.size || lexer.tokens[i+1].type != IDENTIFIER){
							fprintf(stderr, "Function declaration requires name afterwards\n");
							res.exit = ERROR_WRONG_ARGS;
							inFunctionBody = 0;
							break;
						}
						res.functions[res.function_count].offset = lexer.tokens[i+1].offset;
						res.functions[res.function_count].size = lexer.tokens[i+1].size;

						break;
					}
					// FUNCALL
					if(res.size >= res.capacity){
						res.capacity *= 2;
						res.expressions = realloc(res.expressions, res.capacity*sizeof(Token));
					}
					res.expressions[res.size].type = FUN_CALL;
					res.expressions[res.size].as.fun_call = malloc(sizeof(Expr_Fun_Call));
					res.expressions[res.size].as.fun_call->name = token;
					res.expressions[res.size].as.fun_call->argc = 0;
					res.expressions[res.size].as.fun_call->args = malloc(16*sizeof(Expr));
					res.size++;
					inFunctionCall = 1;
				}
				break;
			}
		}
	}

	return res;
}

int stoi(char* src_w_offset, size_t size){
	int res = 0;
	int negative = 1;

	for(int i = 0; i < (int)size; i++){
		int p10 = size-i-1-(negative < 0 ? 1 : 0);
		int shift = 1;
		for(int j = 0; j < p10; j++){
			shift *= 10;
		}

		char c = src_w_offset[i];
		if(i == 0 && c == '-'){
			negative = -1;
			continue;
		}
		res += (shift*(c-'0'));
	}

	return res * negative;
}

int simplify_expression(char* src, Expr expr, Var* vars, int var_count){
	if(expr.type == BOOL){
		int lhs, rhs;
		switch(expr.as.token_op->lhs.type){
			case LITERAL:
			{
				if(expr.as.token_op->lhs.as.literal->type == STRING){
					fprintf(stderr, "simplify_expression: cannot boolean compare a string\n");
					return 0;
				}
				lhs = stoi(src+expr.as.token_op->lhs.as.literal->offset, expr.as.token_op->lhs.as.literal->size);
				break;
			}
			case VAR_REF:
			{
				int found_index = -1;
				char search_name[expr.as.token_op->lhs.as.literal->size];
				strncpy(search_name, src+expr.as.token_op->lhs.as.literal->offset, expr.as.token_op->lhs.as.literal->size);
				for(int j = 0; j < var_count; j++){
					char cmp_name[vars[j].n_size];
					strncpy(cmp_name, src+vars[j].n_offset, vars[j].n_size);
					if(strncmp(search_name, cmp_name, vars[j].n_size) == 0){
						found_index = j;
						break;
					}
				}
				if(found_index < 0){
					fprintf(stderr, "SIMPLIFY_EXPRESSION_VAR_REF: Failed to find variable \"%.*s\"\n", (int)expr.as.token_op->lhs.as.literal->size, search_name);
					return 0;
				}
				if(vars[found_index].type == STRING){
					fprintf(stderr, "simplify_expression: cannot use string for boolean expression\n");
					return 0;
				}
				lhs = vars[found_index].integer;
				break;
			}
			default:
			{
				fprintf(stderr, "Cannot create lhs of boolean expression from type %i\n", expr.as.token_op->lhs.type);
				lhs = 0;
				break;
			}
		}
		switch(expr.as.token_op->rhs.type){
			case LITERAL:
			{
				if(expr.as.token_op->rhs.as.literal->type == STRING){
					fprintf(stderr, "simplify_expression: cannot boolean compare a string\n");
					return 0;
				}
				rhs = stoi(src+expr.as.token_op->rhs.as.literal->offset, expr.as.token_op->rhs.as.literal->size);
				break;
			}
			case VAR_REF:
			{
				int found_index = -1;
				char search_name[expr.as.token_op->rhs.as.literal->size];
				strncpy(search_name, src+expr.as.token_op->rhs.as.literal->offset, expr.as.token_op->rhs.as.literal->size);
				for(int j = 0; j < var_count; j++){
					char cmp_name[vars[j].n_size];
					strncpy(cmp_name, src+vars[j].n_offset, vars[j].n_size);
					if(strncmp(search_name, cmp_name, vars[j].n_size) == 0){
						found_index = j;
						break;
					}
				}
				if(found_index < 0){
					fprintf(stderr, "SIMPLIFY_EXPRESSION_VAR_REF: Failed to find variable \"%.*s\"\n", (int)expr.as.token_op->rhs.as.literal->size, search_name);
					return 0;
				}
				if(vars[found_index].type == STRING){
					fprintf(stderr, "simplify_expression: cannot use string for boolean expression\n");
					return 0;
				}
				rhs = vars[found_index].integer;
				break;
			}
			default:
			{
				fprintf(stderr, "Cannot create rhs of boolean expression from type %i\n", expr.as.token_op->rhs.type);
				rhs = 0;
				break;
			}
		}

		switch(expr.as.token_op->operator.type){
			case LEQ: return lhs <= rhs;
			case LT: return lhs < rhs;
			case GEQ: return lhs >= rhs;
			case GT: return lhs > rhs;
			case EQEQ: return lhs == rhs;
			default: return 0;
		}
	}
	else if(expr.type == MATH){
		int lhs, rhs;
		switch(expr.as.token_op->lhs.type){
			case LITERAL:
			{
				if(expr.as.token_op->lhs.as.literal->type == STRING){
					fprintf(stderr, "simplify_expression: cannot math compare a string\n");
					return 0;
				}
				lhs = stoi(src+expr.as.token_op->lhs.as.literal->offset, expr.as.token_op->lhs.as.literal->size);
				break;
			}
			case VAR_REF:
			{
				int found_index = -1;
				char search_name[expr.as.token_op->lhs.as.literal->size];
				strncpy(search_name, src+expr.as.token_op->lhs.as.literal->offset, expr.as.token_op->lhs.as.literal->size);
				for(int j = 0; j < var_count; j++){
					char cmp_name[vars[j].n_size];
					strncpy(cmp_name, src+vars[j].n_offset, vars[j].n_size);
					if(strncmp(search_name, cmp_name, vars[j].n_size) == 0){
						found_index = j;
						break;
					}
				}
				if(found_index < 0){
					fprintf(stderr, "SIMPLIFY_EXPRESSION_VAR_REF: Failed to find variable \"%.*s\"\n", (int)expr.as.token_op->lhs.as.literal->size, search_name);
					return 0;
				}
				if(vars[found_index].type == STRING){
					fprintf(stderr, "simplify_expression: cannot use string for math expression\n");
					return 0;
				}
				lhs = vars[found_index].integer;
				break;
			}
			default:
			{
				fprintf(stderr, "Cannot create lhs of math expression from type %i\n", expr.as.token_op->lhs.type);
				lhs = 0;
				break;
			}
		}
		switch(expr.as.token_op->rhs.type){
			case LITERAL:
			{
				if(expr.as.token_op->rhs.as.literal->type == STRING){
					fprintf(stderr, "simplify_expression: cannot math compare a string\n");
					return 0;
				}
				rhs = stoi(src+expr.as.token_op->rhs.as.literal->offset, expr.as.token_op->rhs.as.literal->size);
				break;
			}
			case VAR_REF:
			{
				int found_index = -1;
				char search_name[expr.as.token_op->rhs.as.literal->size];
				strncpy(search_name, src+expr.as.token_op->rhs.as.literal->offset, expr.as.token_op->rhs.as.literal->size);
				for(int j = 0; j < var_count; j++){
					char cmp_name[vars[j].n_size];
					strncpy(cmp_name, src+vars[j].n_offset, vars[j].n_size);
					if(strncmp(search_name, cmp_name, vars[j].n_size) == 0){
						found_index = j;
						break;
					}
				}
				if(found_index < 0){
					fprintf(stderr, "SIMPLIFY_EXPRESSION_VAR_REF: Failed to find variable \"%.*s\"\n", (int)expr.as.token_op->rhs.as.literal->size, search_name);
					return 0;
				}
				if(vars[found_index].type == STRING){
					fprintf(stderr, "simplify_expression: cannot use string for math expression\n");
					return 0;
				}
				rhs = vars[found_index].integer;
				break;
			}
			default:
			{
				fprintf(stderr, "Cannot create rhs of math expression from type %i\n", expr.as.token_op->rhs.type);
				rhs = 0;
				break;
			}
		}

		switch(expr.as.token_op->operator.type){
			case PLUS: return lhs + rhs;
			case MINUS: return lhs - rhs;
			case STAR: return lhs * rhs;
			case SLASH: return lhs / rhs;
			default: return 0;
		}
	}
	else{
		fprintf(stderr, "simplify_expression: cannot simplify type %i, returning 0\n", expr.type);
		return 0;
	}

	return 0;
}

Var get_var_assignment(char* src, Expr expr, Var* vars, int var_count){
	Var var = {0};
	var.n_offset = expr.as.var_set->name.offset;
	var.n_size = expr.as.var_set->name.size;
start_switch_get_var_assignment:
	switch(expr.as.var_set->value.type){
		case GROUP:
		{
#ifdef DEBUG
			enum ExprType old = expr.as.var_set->value.type;
#endif
			expr.as.var_set->value = expr.as.var_set->value.as.group->as;
#ifdef DEBUG
			printf("WARN: switching expr.as.var_set->value from type %i to type %i\n", old, expr.as.var_set->value.type);
#endif
			goto start_switch_get_var_assignment;
		}
		case LITERAL:
		{
			var.type = expr.as.var_set->value.as.literal->type;
			if(expr.as.var_set->value.as.literal->type == STRING){
				var.s_offset = expr.as.var_set->value.as.literal->offset;
				var.s_size = expr.as.var_set->value.as.literal->size;
			}
			else{
				var.integer = stoi(src+expr.as.var_set->value.as.literal->offset, expr.as.var_set->value.as.literal->size);
			}
			break;
		}
		case VAR_REF:
		{
			int found_index = -1;
			char search_name[expr.as.var_set->value.as.literal->size];
			strncpy(search_name, src+expr.as.var_set->value.as.literal->offset, expr.as.var_set->value.as.literal->size);
			for(int j = 0; j < var_count; j++){
				char cmp_name[vars[j].n_size];
				strncpy(cmp_name, src+vars[j].n_offset, vars[j].n_size);
				if(strncmp(search_name, cmp_name, vars[j].n_size) == 0){
					found_index = j;
					break;
				}
			}
			if(found_index < 0){
				fprintf(stderr, "VAR_SET_VAR_REF: Failed to find variable \"%.*s\"\n", (int)expr.as.var_set->value.as.literal->size, search_name);
				return (Var){.n_offset = 0, .n_size = 0, .type = 0, .integer = 0, .s_offset = 0, .s_size = 0};
			}

			var.type = vars[found_index].type;
			if(var.type == STRING){
				var.s_offset = vars[found_index].s_offset;
				var.s_size = vars[found_index].s_size;
			}
			else{
				var.integer = vars[found_index].integer;
			}
			break;
		}
		case BOOL:
		case MATH:
		{
			var.type = NUMBER;
			var.integer = simplify_expression(src, expr.as.var_set->value, vars, var_count);
			break;
		}
		default:
		{
			printf("WARN: not implemented variable assignment for type %i\n", expr.as.var_set->value.type);
			var.type = NUMBER;
			var.integer = 0;
			break;
		}
	};

	return var;
}
	
int eval_expressions(char* src, Expr* expressions, int size){
	int exit_code = 0;

	Var* vars = malloc(8*sizeof(Var));
	int var_count = 0;
	int var_capacity = 8;
	int loop_start = -1;
	for(int i = 0; i < size; i++){
		Expr expr = expressions[i];
start_switch_expr:
		switch(expr.type){
			case GROUP:
			{
				expr = expr.as.group->as;
				goto start_switch_expr;
				break;
			}
			case VAR_SET:
			{
				if(var_count >= var_capacity){
					var_capacity *= 2;
					vars = realloc(vars, var_capacity*sizeof(Var));
				}

				vars[var_count] = get_var_assignment(src, expr, vars, var_count);

				var_count++;
				break;
			}
			case VAR_REASSIGN:
			{
				int found_index = -1;
				char search_name[expr.as.literal->size];
				strncpy(search_name, src+expr.as.literal->offset, expr.as.literal->size);
				for(int j = 0; j < var_count; j++){
					char cmp_name[vars[j].n_size];
					strncpy(cmp_name, src+vars[j].n_offset, vars[j].n_size);
					if(strncmp(search_name, cmp_name, vars[j].n_size) == 0){
						found_index = j;
						break;
					}
				}
				if(found_index < 0){
					fprintf(stderr, "VAR_REASSIGN: Failed to find variable \"%.*s\"\n", (int)expr.as.literal->size, search_name);
					exit_code = ERROR_DNE;
					goto finish_eval;
				}
				
				vars[found_index] = get_var_assignment(src, expr, vars, var_count);

				break;
			}
			case FUN_CALL:
			{
				switch(expr.as.fun_call->name.type){
					case CALL:
					{
						// TODO loop through and find function to call
						break;
					}
					case EXIT:
					{
						if(expr.as.fun_call->argc > 0
						&& expr.as.fun_call->args[0].type == LITERAL
						&& expr.as.fun_call->args[0].as.literal->type == NUMBER){
							exit_code = stoi(src+expr.as.fun_call->args[0].as.literal->offset, expr.as.fun_call->args[0].as.literal->size);
						}
						else{
							exit_code = 1;
						}
						goto finish_eval;
						break;
					}
					case FOR:
					{
						if(expr.as.fun_call->argc < 2){
							fprintf(stderr, "Incorrect number of args for for loop\n");
							exit_code = ERROR_WRONG_ARGS;
							goto finish_eval;
						}
						loop_start = i;
						break;
					}
					case END:
					{
						if(loop_start < 0){
							fprintf(stderr, "Stray \"end\" call made\n");
							exit_code = ERROR_EXTRA;
							goto finish_eval;
						}
						int found_index = -1;
						char search_name[expressions[loop_start].as.fun_call->args[0].as.literal->size];
						strncpy(search_name, src+expressions[loop_start].as.fun_call->args[0].as.literal->offset, expressions[loop_start].as.fun_call->args[0].as.literal->size);
						for(int j = 0; j < var_count; j++){
							char cmp_name[vars[j].n_size];
							strncpy(cmp_name, src+vars[j].n_offset, vars[j].n_size);
							if(strncmp(search_name, cmp_name, vars[j].n_size) == 0){
								found_index = j;
								break;
							}
						}
						if(found_index < 0){
							fprintf(stderr, "END: Failed to find variable \"%.*s\"\n", (int)expressions[loop_start].as.fun_call->args[0].as.literal->size, search_name);
							exit_code = ERROR_DNE;
							goto finish_eval;
						}

						if(vars[found_index].type != NUMBER){
							fprintf(stderr, "For loop requires variable to be integer not string\n");
							exit_code = ERROR_WRONG_TYPE;
							goto finish_eval;
						}
						if(expressions[loop_start].as.fun_call->args[1].type != LITERAL){
							fprintf(stderr, "Need upper bound for loop\n");
							exit_code = ERROR_WRONG_ARGS;
							goto finish_eval;
						}
						if(expressions[loop_start].as.fun_call->args[1].as.literal->type != NUMBER){
							fprintf(stderr, "For loop upper bound needs to be integer\n");
							exit_code = ERROR_WRONG_TYPE;
							goto finish_eval;
						}
						if(vars[found_index].integer < stoi(src+expressions[loop_start].as.fun_call->args[1].as.literal->offset, expressions[loop_start].as.fun_call->args[1].as.literal->size)){
							i = loop_start;
							vars[found_index].integer++;
						}
						else{
							loop_start = -1;
						}
						break;
					}
					case PRINT:
					{	
						printf("[PROGRAM OUTPUT] ");
						for(int i = 0; i < (int)expr.as.fun_call->argc; i++){
							switch(expr.as.fun_call->args[i].type){
								case LITERAL:
								{
									printf("%.*s", (int)expr.as.fun_call->args[i].as.literal->size, src+expr.as.fun_call->args[i].as.literal->offset);
									break;
								}
								case VAR_REF:
								{
									int found_index = -1;
									char search_name[expr.as.fun_call->args[i].as.literal->size];
									strncpy(search_name, src+expr.as.fun_call->args[i].as.literal->offset, expr.as.fun_call->args[i].as.literal->size);
									for(int j = 0; j < var_count; j++){
										char cmp_name[vars[j].n_size];
										strncpy(cmp_name, src+vars[j].n_offset, vars[j].n_size);
										if(strncmp(search_name, cmp_name, vars[j].n_size) == 0){
											found_index = j;
											break;
										}
									}
									if(found_index < 0){
										fprintf(stderr, "PRINT_VAR_REF: Failed to find variable \"%.*s\"\n", (int)expr.as.fun_call->args[i].as.literal->size, search_name);
										exit_code = ERROR_DNE;
										goto finish_eval;
									}
									if(vars[found_index].type == STRING){
										printf("%.*s", (int)vars[found_index].s_size, src+vars[found_index].s_offset);
									}
									else{
										printf("%d", vars[found_index].integer);
									}
									break;
								}
								default:
								{
									fprintf(stderr, "\nExpr type %i not supported for if statements yet\n", expr.as.fun_call->args[i].type);
									i = (int)expr.as.fun_call->argc;
									break;
								}
							}
						}
						printf("\n");
						break;
					}
					default:
					{
						fprintf(stderr, "Function %i not implemented yet\n", expr.as.fun_call->name.type);
						exit_code = ERROR_UNKNOWN;
						goto finish_eval;
						break;
					}
				}
				break;
			}
			default:
			{
				fprintf(stderr, "Unimplemented expression type: %i\n", expr.type);
				i = size;
				break;
			}
		}
	}

finish_eval:
	free(vars);
	vars = NULL;

	return exit_code;
}

int run_code(char* src, int size){
	src = preprocess(src, &size);
	if(src == NULL){
		fprintf(stderr, "run_code: Failed to preprocess src\n");
		return 1;
	}

	Lexer lexer = lex(src, size);
	Parser parser = {0};

	if(lexer.exit != ERROR_NONE){
		printf("Lexer: Error Code: %i\n", lexer.exit);
		goto end; // error in lexing, do not go on
	}
#ifdef DEBUG
	printf("----\n");
	for(int i = 0; i < (int)lexer.size; i++){
		if(lexer.tokens[i].type == NEWLINE){
			printf("Token %i: [Type: %i] \"\\n\"\n", i, lexer.tokens[i].type);
		}
		else{
			printf("Token %i: [Type: %i] \"%.*s\"\n", i, lexer.tokens[i].type, (int)lexer.tokens[i].size, src+lexer.tokens[i].offset);
		}
	}
	printf("----\n");
#endif

	parser = parse(lexer);
	if(parser.exit != ERROR_NONE){
		printf("Parser: Error Code: %i\n", parser.exit);
		goto end; // error in parsing, do not go on
	}

#ifdef DEBUG
	for(int i = 0; i < (int)parser.size; i++){
		print_expression(src, "", parser.expressions[i]);
	}
	printf("--RUNNING CODE TIME--\n");
#endif

	int exit_code = eval_expressions(src, parser.expressions, (int)parser.size);

end:
	if(parser.expressions != NULL){
		for(int i = 0; i < (int)parser.size; i++){
			switch(parser.expressions[i].type){
				case BOOL:
				case MATH:
				{
					free(parser.expressions[i].as.token_op);
					break;
				}
				case GROUP:
				{
					free(parser.expressions[i].as.group);
					break;
				}
				case VAR_SET:
				case VAR_REASSIGN:
				{
					free(parser.expressions[i].as.var_set);
					break;
				}
				case FUN_CALL:
				{
					free(parser.expressions[i].as.fun_call->args);
					free(parser.expressions[i].as.fun_call);
					break;
				}
				default: break;
			};
		}
		free(parser.expressions);
		parser.expressions = NULL;
	}
	free(lexer.tokens);
	lexer.tokens = NULL;
	free(src);
	return exit_code;
}

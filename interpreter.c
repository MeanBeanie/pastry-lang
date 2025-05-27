#include "interpreter.h"
#include "lexer.h"
#include "parser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int get_digits(int value){
	int res = 1;
	while(value > 0){
		value /= 10;
		res++;
	}

	return res;
}

Var find_var(Var** vars, size_t vsize, char* name, size_t size){
	for(size_t i = 0; i < vsize; i++){
		if(strncmp(name, (*vars)[i].name, size) == 0){
			return (*vars)[i];
		}
	}

	Var var = {0};
	return var;
}

void add_var(Var** vars, size_t* size, size_t* capacity, Var var){
	if(*size >= *capacity){
		*capacity *= 2;
		*vars = realloc((*vars), (*capacity)*sizeof(Var));
	}

	(*vars)[*size] = var;
	*size = (*size) + 1;
}

void update_var(Var** vars, size_t var_count, char* str, size_t size, Var var){
	for(size_t i = 0; i < var_count; i++){
		if(strncmp(str, (*vars)[i].name, size) == 0){
			(*vars)[i] = var;
			break;
		}
	}
}

int solve_expr(Var** vars, size_t size, Expr expr){
	if(expr.type == GROUPED){
		expr = expr.as.grouped->expr;
	}

	if(expr.type == LITERAL){
		return 1;
	}

	if(expr.type != OPERATION){
		return 0;
	}

	Expr lhs = expr.as.operation->lhs;
	while(lhs.type != LITERAL){
		if(lhs.type == GROUPED){
			lhs = lhs.as.grouped->expr;
			continue;
		}
		if(lhs.type == OPERATION){
			int value = solve_expr(vars, size, lhs);
			lhs.type = LITERAL;
			lhs.as.literal = malloc(sizeof(Token));
			lhs.as.literal->type = INTEGER;
			int digits = get_digits(value);
			lhs.as.literal->str = malloc((digits+1)*sizeof(char));
			snprintf(lhs.as.literal->str, digits, "%d", value);
			lhs.as.literal->str[digits] = '\0';
			lhs.as.literal->size = strlen(lhs.as.literal->str);
		}
	}
	if(lhs.as.literal->type == IDENTIFIER){
		Var var = find_var(vars, size, lhs.as.literal->str, lhs.as.literal->size);
		if(var.str == NULL){
			fprintf(stderr, "[ERR] Failed to find var %.*s\n", (int)lhs.as.literal->size, lhs.as.literal->str);
			return 0;
		}
		lhs.as.literal->type = var.type;
		lhs.as.literal->str = var.str;
		lhs.as.literal->size = var.str_size;
	}
	
	Expr rhs = expr.as.operation->rhs;
	while(rhs.type != LITERAL){
		if(rhs.type == GROUPED){
			rhs = rhs.as.grouped->expr;
			continue;
		}
		if(rhs.type == OPERATION){
			int value = solve_expr(vars, size, rhs);
			rhs.type = LITERAL;
			rhs.as.literal = malloc(sizeof(Token));
			rhs.as.literal->type = INTEGER;
			int digits = get_digits(value);
			rhs.as.literal->str = malloc((digits+1)*sizeof(char));
			snprintf(rhs.as.literal->str, digits, "%d", value);
			rhs.as.literal->str[digits] = '\0';
			rhs.as.literal->size = strlen(rhs.as.literal->str);
		}
	}
	if(rhs.as.literal->type == IDENTIFIER){
		Var var = find_var(vars, size, rhs.as.literal->str, rhs.as.literal->size);
		if(var.str == NULL){
			fprintf(stderr, "[ERR] Failed to find var %.*s\n", (int)rhs.as.literal->size, rhs.as.literal->str);
			return 0;
		}
		rhs.as.literal->type = var.type;
		rhs.as.literal->str = var.str;
		rhs.as.literal->size = var.str_size;
	}

	if(lhs.as.literal->type != rhs.as.literal->type){
		fprintf(stderr, "[ERR] Cannot operate on two different types\n");
		return 0;
	}
	
	if(expr.as.operation->operator >= EQEQ && expr.as.operation->operator <= GTEQ){
		// bool
		if(lhs.as.literal->type == STRING){
			int value = strcmp(lhs.as.literal->str, rhs.as.literal->str);
			switch(expr.as.operation->operator){
				case EQEQ: return value == 0;
				case LT: return value < 0;
				case LTEQ: return value <= 0;
				case GT: return value > 0;
				case GTEQ: return value >= 0;
				default: return 0;
			}
		}
		else{
			int l = atoi(lhs.as.literal->str);
			int r = atoi(rhs.as.literal->str);
			switch(expr.as.operation->operator){
				case EQEQ: return l == r;
				case LT: return l < r;
				case LTEQ: return l <= r;
				case GT: return l > r;
				case GTEQ: return l >= r;
				default: return 0;
			}
		}
	}
	else if(expr.as.operation->operator >= PLUS && expr.as.operation->operator <= SLASH){
		// math
		if(lhs.as.literal->type == STRING){
			fprintf(stderr, "[ERR] Cannot do non-boolean operations on strings\n");
			return 0;
		}
		else{
			int l = atoi(lhs.as.literal->str);
			int r = atoi(rhs.as.literal->str);
			switch(expr.as.operation->operator){
				case PLUS: return l + r;
				case MINUS: return l - r;
				case STAR: return l * r;
				case SLASH: return l / r;
				default: return 0;
			}
		}
	}

	return 0;
}

int eval_expressions(Parser parser, Expr* exprs, size_t size){
	int exit_code = 0;
	size_t var_count = 0;
	size_t var_cap = 8;
	Var* vars = malloc(var_cap*sizeof(Var));

	for(size_t i = 0; i < size; i++){
		Expr expr = exprs[i];
		switch(expr.type){
			case FUNCTION_CALL:
			{
				switch(expr.as.function_call->type){
					case CALL:
					{
						if(expr.as.function_call->argc < 1){
							fprintf(stderr, "[ERR] Call function requires function name to be an argument\n");
							break;
						}
						if(expr.as.function_call->argv[0].type != LITERAL){
first_arg_func_name_call:
							fprintf(stderr, "[ERR] Call function requires function name in first argument\n");
							break;
						}
						if(expr.as.function_call->argv[0].as.literal->type != IDENTIFIER){
							goto first_arg_func_name_call;
						}

						Token* name = expr.as.function_call->argv[0].as.literal;
						int found_index = -1;
						for(size_t j = 0; j < parser.function_count; j++){
							if(strncmp(name->str, parser.functions[j].name, name->size) == 0){
								found_index = (int)j;
								break;
							}
						}
						if(found_index < 0){
							fprintf(stderr, "[ERR] Function %.*s does not exist\n", (int)name->size, name->str);
							break;
						}

						int param_count = expr.as.function_call->argc-1;
						if(param_count != (int)parser.functions[found_index].argc){
							fprintf(stderr, "[ERR] Call function needs all parameters required by function being called\n");
							break;
						}
						size_t func_size = param_count + parser.functions[found_index].size;
						Expr* func_exprs = malloc(sizeof(Expr)*func_size);
						for(size_t j = 0; j < func_size; j++){
							if((int)j >= param_count){
								func_exprs[j] = parser.functions[found_index].exprs[j-param_count];
							}
							else{
								Var var = find_var(&vars, var_count, expr.as.function_call->argv[j+1].as.literal->str, expr.as.function_call->argv[j+1].as.literal->size);
								if(var.str == NULL){
									fprintf(stderr, "[ERR] Cannot find variable %s\n", expr.as.function_call->argv[j+1].as.literal->str);
									goto end_call_function_call;
								}
								Expr param_expr = {0};
								param_expr.type = FUNCTION_CALL;
								param_expr.as.function_call = malloc(sizeof(struct Expr_Function_Call));
								param_expr.as.function_call->type = VAR;
								param_expr.as.function_call->argc = 2;
								param_expr.as.function_call->argv = malloc(sizeof(Expr)*2);

								param_expr.as.function_call->argv[0].type = LITERAL;
								param_expr.as.function_call->argv[0].as.literal = malloc(sizeof(Token));
								param_expr.as.function_call->argv[0].as.literal->type = IDENTIFIER;
								param_expr.as.function_call->argv[0].as.literal->size = parser.functions[found_index].argv[j].size;
								param_expr.as.function_call->argv[0].as.literal->str = malloc(sizeof(char)*(parser.functions[found_index].argv[j].size+1));
								strncpy(param_expr.as.function_call->argv[0].as.literal->str, parser.functions[found_index].argv[j].str, parser.functions[found_index].argv[j].size);
								param_expr.as.function_call->argv[0].as.literal->str[parser.functions[found_index].argv[j].size] = '\0';

								param_expr.as.function_call->argv[1].type = LITERAL;
								param_expr.as.function_call->argv[1].as.literal = malloc(sizeof(Token));
								param_expr.as.function_call->argv[1].as.literal->type = var.type;
								param_expr.as.function_call->argv[1].as.literal->size = var.str_size;
								param_expr.as.function_call->argv[1].as.literal->str = malloc(sizeof(char)*(var.str_size+1));
								strncpy(param_expr.as.function_call->argv[1].as.literal->str, var.str, var.str_size);
								param_expr.as.function_call->argv[1].as.literal->str[var.str_size] = '\0';

								func_exprs[j] = param_expr;
							}
						}

						int func_exit_code = eval_expressions(parser, func_exprs, func_size);
						if(func_exit_code != 0){
							exit_code = func_exit_code;
						}

end_call_function_call:

						free(func_exprs);

						break;
					}
					case PRINT:
					{
						for(size_t j = 0; j < expr.as.function_call->argc; j++){
							Expr arg = expr.as.function_call->argv[j];
							switch(expr.as.function_call->argv[i].type){
								case OPERATION: printf("%d", solve_expr(&vars, var_count, arg)); break;
								case LITERAL:
								{
									if(arg.as.literal->type != IDENTIFIER){
										printf("%s", arg.as.literal->str);
									}
									else{
										Var var = find_var(&vars, var_count, arg.as.literal->str, arg.as.literal->size);
										printf("%s", var.str);
									}
									break;
								}
								case GROUPED:
								{
									if(arg.as.grouped->expr.type == OPERATION){
										printf("%d", solve_expr(&vars, var_count, arg.as.grouped->expr));
									}
									else{
										printf("%s", arg.as.grouped->expr.as.literal->str);
									}
									break;
								}
								default: break;
							}
						}
						printf("\n");
						break;
					}
					case VAR:
					{
						if(expr.as.function_call->argc != 2){
							fprintf(stderr, "[ERR] Var call requires 2 args, the variable and the value\n");
							break;
						}
						if(expr.as.function_call->argv[0].type != LITERAL){
first_arg_name_error:
							fprintf(stderr, "[ERR] Var call requires first arg to be var name\n");
							break;
						}
						Token* name = expr.as.function_call->argv[0].as.literal;
						if(name->type != IDENTIFIER){
							goto first_arg_name_error;
						}

						Expr arg2 = expr.as.function_call->argv[1];
set_value:
						Token value;
						if(arg2.type == LITERAL){
							value = *arg2.as.literal;
							if(value.type == IDENTIFIER){
								Var rhs = find_var(&vars, var_count, value.str, value.size);
								if(rhs.str == NULL){
									fprintf(stderr, "[ERR] Cannot find variable %s\n", value.str);
									break;
								}
								value.type = rhs.type;
								value.size = rhs.str_size;
								free(value.str);
								value.str = malloc(sizeof(char)*(rhs.str_size+1));
								strncpy(value.str, rhs.str, rhs.str_size);
								value.str[rhs.str_size] = '\0';
							}
						}
						else if(arg2.type == GROUPED){
							arg2 = arg2.as.grouped->expr;
							goto set_value;
						}
						else if(arg2.type == OPERATION){
							int v = solve_expr(&vars, var_count, arg2);
							int digits = get_digits(v);
							value.str = malloc((digits+1)*sizeof(char));
							snprintf(value.str, digits, "%d", v);
							value.str[digits] = '\0';
							value.size = strlen(value.str);
							value.type = INTEGER;
						}
						else{
							fprintf(stderr, "[ERR] Var call cannot take function calls in the arguments\n");
							break;
						}

						Var var = find_var(&vars, var_count, name->str, name->size);
						if(var.str != NULL){
							var.str_size = value.size;
							var.str = realloc(var.str, sizeof(char)*(value.size+1));
							strncpy(var.str, value.str, value.size);
							var.str[value.size] = '\0';
							var.type = value.type;
							update_var(&vars, var_count, name->str, name->size, var);
						}
						else{
							var.str_size = value.size;
							var.str = malloc(sizeof(char)*(value.size+1));
							strncpy(var.str, value.str, value.size);
							var.str[value.size] = '\0';
							var.type = value.type;
							var.name_size = name->size;
							var.name = malloc(sizeof(char)*(name->size+1));
							strncpy(var.name, name->str, name->size);
							var.name[name->size] = '\0';
							add_var(&vars, &var_count, &var_cap, var);
						}
						break;
					}
					default: break;
				}
				break;
			}
			default: break;
		}
	}

	return exit_code;
}

int run_code(char* src, size_t size, int debug_mode){
	int exit_code = 0;
	Parser parser = {0};
	Lexer lexer = lex(src, size);
	if(debug_mode == 0){
		print_lexer(lexer);
	}
	if(lexer.exit_code != 0){
		printf("[INFO] Lexer had error, stopping here\n");
		exit_code = lexer.exit_code;
		goto finish_running;
	}

	parser = parse(lexer);
	if(debug_mode == 0){
		print_parser(parser);
	}
	if(parser.exit_code != 0){
		printf("[INFO] Parser had error, stopping here\n");
		exit_code = parser.exit_code;
		goto finish_running;
	}

	exit_code = eval_expressions(parser, parser.exprs, parser.size);


finish_running:
	if(parser.exprs != NULL){
		free_parser(&parser);
	}
	free_lexer(&lexer);
	return exit_code;
}

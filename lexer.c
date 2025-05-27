#include "lexer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

void incr_size(Lexer* ptr){
	ptr->size++;
	if(ptr->size >= ptr->capacity){
		ptr->capacity *= 2;
		ptr->tokens = realloc(ptr->tokens, ptr->capacity*sizeof(Token));
	}
}

enum TokenType check_for_reserved(char* src, int offset, int size){
	char str[size];
	strncpy(str, src+offset, size);

	if(strncmp(str, "var", strlen("var")) == 0){ return VAR; }
	if(strncmp(str, "print", strlen("print")) == 0){ return PRINT; }
	if(strncmp(str, "read", strlen("read")) == 0){ return READ; }
	if(strncmp(str, "for", strlen("for")) == 0){ return FOR; }
	if(strncmp(str, "while", strlen("while")) == 0){ return WHILE; }
	if(strncmp(str, "if", strlen("if")) == 0){ return IF; }
	if(strncmp(str, "else", strlen("else")) == 0){ return ELSE; }
	if(strncmp(str, "elif", strlen("elif")) == 0){ return ELIF; }
	if(strncmp(str, "and", strlen("and")) == 0){ return AND; }
	if(strncmp(str, "or", strlen("or")) == 0){ return OR; }
	if(strncmp(str, "not", strlen("not")) == 0){ return NOT; }
	if(strncmp(str, "exit", strlen("exit")) == 0){ return EXIT; }
	if(strncmp(str, "end", strlen("end")) == 0){ return END; }
	if(strncmp(str, "func", strlen("func")) == 0){ return FUNC; }
	if(strncmp(str, "call", strlen("call")) == 0){ return CALL; }

	return IDENTIFIER;
}

void add_token(Lexer* ptr, char* src, enum TokenType type, int offset, int size){
	if(type == IDENTIFIER){
		type = check_for_reserved(src, offset, size);
	}
	ptr->tokens[ptr->size] = (Token){
		.type = type,
		.size = size,
		.str = malloc(sizeof(char)*(size+1))
	};
	strncpy(ptr->tokens[ptr->size].str, src+offset, size);
	ptr->tokens[ptr->size].str[size] = '\0';

	incr_size(ptr);
}

Lexer lex(char* src, size_t size){
	Lexer res = {
		.size = 0,
		.capacity = 8,
		.tokens = malloc(8*sizeof(Token)),
		.exit_code = 0,
	};

	int line = 1;
	int inSomething = 0;
	int something_size = 0;
	for(size_t i = 0; i < size; i++){
		char c = src[i];

		if(inSomething == 1){ // in comment
			if(c != '\n'){
				continue;
			}
			inSomething = 0;
			continue;
		}
		else if(inSomething == 2){ // in int
			if(isdigit(c)){
				something_size++;
				continue;
			}
			add_token(&res, src, INTEGER, i-something_size-1, something_size+1);
			inSomething = 0;
		}
		else if(inSomething == 3){ // in identifier
			if(isalnum(c)){
				something_size++;
				continue;
			}
			add_token(&res, src, IDENTIFIER, i-something_size-1, something_size+1);
			inSomething = 0;
		}
		else if(inSomething == 4){ // in str
			if(c != '"'){
				something_size++;
				continue;
			}
			add_token(&res, src, STRING, i-something_size, something_size);
			inSomething = 0;
			continue;
		}
		switch(c){
			case ' ':
			case '\t':
			case '\r':
			{
				// ignore whitespace
				break;
			}
			case '\n':
			{
				add_token(&res, src, NEWLINE, i, 1);
				line++;
				break;
			}
			case '+':
			{
				add_token(&res, src, PLUS, i, 1);
				break;
			}
			case '-':
			{
				add_token(&res, src, MINUS, i, 1);
				break;
			}
			case '*':
			{
				add_token(&res, src, STAR, i, 1);
				break;
			}
			case ',':
			{
				add_token(&res, src, COMMA, i, 1);
				break;
			}
			case '(':
			{
				add_token(&res, src, GROUP_START, i, 1);
				break;
			}
			case ')':
			{
				add_token(&res, src, GROUP_END, i, 1);
				break;
			}
			case '/':
			{
				if(i+1 < size && src[i+1] == '/'){
					inSomething = 1;
					break;
				}
				add_token(&res, src, SLASH, i, 1);
				break;
			}
			case '=':
			{
				if(i+1 < size && src[i+1] == '='){
					add_token(&res, src, EQEQ, i, 2);
					i += 2;
					break;
				}
				fprintf(stderr, "[ERR][line %i] Single equals not a needed thing\n", line);
				break;
			}
			case '>':
			{
				if(i+1 < size && src[i+1] == '='){
					add_token(&res, src, GTEQ, i, 2);
					i += 2;
					break;
				}
				add_token(&res, src, GT, i, 1);
				break;
			}
			case '<':
			{
				if(i+1 < size && src[i+1] == '='){
					add_token(&res, src, LTEQ, i, 2);
					i += 2;
					break;
				}
				add_token(&res, src, LT, i, 1);
				break;
			}
			default:
			{
				if(isdigit(c)){
					something_size = 0;
					inSomething = 2;
					break;
				}
				else if(isalpha(c)){
					something_size = 0;
					inSomething = 3;
					break;
				}
				else if(c == '"'){
					something_size = 0;
					inSomething = 4;
					break;
				}
				ERROR_LOG(res, "[ERR][line %i] Unknown character found: \'%c\'\n", line, c);
				i = size;
				break;
			}
		};
	}

	return res;
}

void print_lexer(Lexer lexer){
	if(lexer.tokens == NULL){
		printf("[DEBG] Lexer is empty\n");
		return;
	}

	printf("--Lexer--\n");
	for(int i = 0; i < (int)lexer.size; i++){
		printf("[DEBG] Token %i, Type: %i, Str: \"%s\"\n", i, lexer.tokens[i].type, lexer.tokens[i].str);
	}
}

void free_lexer(Lexer* lexer){
	for(int i = 0; i < (int)lexer->size; i++){
		free(lexer->tokens[i].str);
	}
	free(lexer->tokens);
	lexer->tokens = NULL;
}

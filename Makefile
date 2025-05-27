FLAGS = -std=c99 -Wall -Wextra -ggdb

frosting: main.o interpreter.o lexer.o parser.o
	gcc -o frosting *.o $(FLAGS)

main.o: main.c
	gcc -c main.c -o main.o $(FLAGS)

interpreter.o: interpreter.c interpreter.h
	gcc -c interpreter.c -o interpreter.o $(FLAGS)

parser.o: parser.c parser.h
	gcc -c parser.c -o parser.o $(FLAGS)

lexer.o: lexer.c lexer.h
	gcc -c lexer.c -o lexer.o $(FLAGS)

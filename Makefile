FLAGS = -std=c99 -Wall -Wextra -ggdb

frosting: main.o interpreter.o
	gcc -o frosting main.o interpreter.o $(FLAGS)

main.o: main.c
	gcc -c main.c -o main.o $(FLAGS)

interpreter.o: interpreter.c interpreter.h
	gcc -c interpreter.c -o interpreter.o $(FLAGS)

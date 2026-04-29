INCLUDES = -I./include

all: main

main: ./src/main.c lexer.o parser.o
	$(CC) $(INCLUDES) -ggdb ./src/main.c parser.o lexer.o -o main

lexer.o: ./src/lexer.c
	$(CC) $(INCLUDES) -ggdb ./src/lexer.c -c
	
parser.o: ./src/parser.c
	$(CC) $(INCLUDES) -ggdb ./src/parser.c -c
	

INCLUDES = -I./include
FLAGS    = -fsanitize=address -ggdb

all: main

main: ./src/main.c lexer.o parser.o
	$(CC) $(INCLUDES) $(FLAGS) ./src/main.c parser.o lexer.o -o main

lexer.o: ./src/lexer.c
	$(CC) $(INCLUDES) $(FLAGS) -ggdb ./src/lexer.c -c
	
parser.o: ./src/parser.c
	$(CC) $(INCLUDES) $(FLAGS) -ggdb ./src/parser.c -c
	

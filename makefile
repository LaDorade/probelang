INCLUDES = -I./include
FLAGS    = -fsanitize=address

all: main

main: ./src/main.c lexer.o parser.o
	$(CC) $(FLAGS) $(INCLUDES) ./src/main.c parser.o lexer.o -o main

lexer.o: ./src/lexer.c
	$(CC) $(FLAGS) $(INCLUDES) ./src/lexer.c -c
	
parser.o: ./src/parser.c
	$(CC) $(FLAGS) $(INCLUDES) ./src/parser.c -c
	

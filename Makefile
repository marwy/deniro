TARGET = deniro
LIBS =
CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -fdiagnostics-color=auto # or ..-color=always

.PHONY: default all clean
.PRECIOUS: $(TARGET)

default: $(TARGET)
all: default

$(TARGET): main.o http.o rules_parser.o server.o
	$(CC) main.o http.o rules_parser.o server.o -Wall $(LIBS) -o $(TARGET)


main.o: main.c
http.o: http.c
rules_parser.o: rules_parser.c
server.o: server.c

test: http.o rules_parser.o server.o test_http.o test_server.o test_rules_parser.o test_runner.o
	$(CC) http.o rules_parser.o server.o test_http.o test_server.o test_rules_parser.o test_runner.o  -Wall $(LIBS) -o test_$(TARGET)
	./test_$(TARGET)

memcheck: all
	valgrind ./$(TARGET) --port 8008 --rules_file=test_rules.txt 2>&1

clean:
	-rm -f *.o
	-rm -f $(TARGET)
	-rm -f test_$(TARGET)

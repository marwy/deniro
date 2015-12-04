TARGET = deniro
LIBS =
CC = gcc
CFLAGS = -g -Wall

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

test: http.o rules_parser.o server.o test_http.o test_server.o test_runner.o
	$(CC) http.o rules_parser.o server.o test_http.o test_server.o test_runner.o  -Wall $(LIBS) -o test_$(TARGET)
	./test_$(TARGET)

clean:
	-rm -f *.o
	-rm -f $(TARGET)
	-rm -f test_$(TARGET)

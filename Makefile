
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Werror -g

TARGET = bfckd
SRC = src/util.c src/main.c 

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean

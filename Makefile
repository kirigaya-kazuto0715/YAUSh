CC = gcc
CFLAGS = -Wall -g
TARGET = shell

all: $(TARGET)

$(TARGET): main.c builtin.c parse.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c builtin.c parse.c

clean:
	rm -f $(TARGET)

CC = gcc
CFLAGS = -Wall -pthread
TARGET = assign03

all: $(TARGET)

$(TARGET): main.o scheduler.o
	$(CC) $(CFLAGS) -o $(TARGET) main.o scheduler.o

main.o: main.c scheduler.h
	$(CC) $(CFLAGS) -c main.c

scheduler.o: scheduler.c scheduler.h
	$(CC) $(CFLAGS) -c scheduler.c

clean:
	rm -f $(TARGET) *.o assign03

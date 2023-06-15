TARGET = main.exe
LIBS = -lm -lc -lfaststdio

CC = gcc-13
CFLAGS = -g -Og -Wall -Wextra -Wno-infinite-recursion

.PHONY: default all clean
.PRECIOUS: $(TARGET) $(OBJECTS)

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard [^_]*.c)) $(patsubst %.cb, %.o, $(wildcard */[^_]*.c))
HEADERS = $(wildcard [^_]*.h) $(wildcard */[^_]*.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@


$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)

# sudo cp ./easyio/libfaststdio.so /usr/local/lib
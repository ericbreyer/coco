TARGET = main.exe
LIBS =

CC = gcc
COCOFLAGS = -D_FORTIFY_SOURCE=0 # -fno-move-loop-invariants
ifdef noassert
COCOFLAGS += -DNDEBUG
endif
CFLAGS = -g -Wall -Wextra -O3 $(COCOFLAGS)

.PHONY: default all clean force
.PRECIOUS: $(TARGET) $(OBJECTS)

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard [^_]*.c)) $(patsubst %.cb, %.o, $(wildcard */[^_]*.c))
HEADERS = $(wildcard [^_]*.h) $(wildcard */[^_]*.h)


%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@


$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@ 

unsafe:
	make clean
	make noassert=1

force:
	make clean
	make

clean:
	-rm -f *.o
	-rm -f $(TARGET)

# sudo cp ./easyio/libfaststdio.so /usr/local/lib
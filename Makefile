CC := gcc
SRCDIR := src
OBJDIR := obj
DEPDIR := include
TARGET := huffman
CFLAGS := -Wall -Wextra -Wpedantic -std=c17

LIBS := 

_OBJS := huffman.o main.o

OBJS := $(patsubst %,$(OBJDIR)/%,$(_OBJS))
_DEPS := huffman.h
DEPS := $(patsubst %,$(DEPDIR)/%,$(_DEPS)) obj

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

obj:
	mkdir $(OBJDIR)

.PHONY: clean

clean:
	rm -rf $(OBJDIR)/*.o $(TARGET) $(OBJDIR) 
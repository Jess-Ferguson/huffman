CC := gcc
SRCDIR := src
OBJDIR := obj
DEPDIR := include
TARGET := huffman
CFLAGS := -Wall -Wextra -Wpedantic -std=c17 -I$(DEPDIR) -g

LIBS := 

_OBJS := huffman.o main.o

OBJS := $(patsubst %,$(OBJDIR)/%,$(_OBJS))
_DEPS := huffman.h
DEPS := $(patsubst %,$(DEPDIR)/%,$(_DEPS))

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPS) $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJDIR):
	mkdir $(OBJDIR)

.PHONY: clean

clean:
	rm -rf $(OBJDIR)/*.o $(TARGET) $(OBJDIR) 
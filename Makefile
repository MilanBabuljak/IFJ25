# Makefile for IFJ25 Compiler
#
# Author: Milan Babuljak (xbabulm00)
# Date: 2. 12. 2025
# Description: Makefile for building the IFJ25 compiler
#

CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -pedantic -g
TARGET=ifj25
SOURCES=main.c string.c lexer.c debug_utils.c ast.c symtable.c stack.c ast-visualiser.c codegen.c builtin.c semantic.c semtree.c
OBJECTS=$(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJECTS) $(TARGET)

clear:
	rm -f $(OBJECTS) $(TARGET)

# Run the compiler
run: $(TARGET)
	./$(TARGET) test_src/random.txt

# Phony targets
.PHONY: all clean run
# Define compiler and flags
CC = gcc
CFLAGS = -g -O0

# List all source files
SOURCES = sim.c decode.c execute.c utils.c shell.c
# Create a list of object files from the source files
OBJECTS = $(SOURCES:.c=.o)
TARGET = sim

# Default target: build the executable
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Pattern rule to compile each .c file into a .o file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule: remove all generated files
.PHONY: clean
clean:
	rm -f $(OBJECTS) $(TARGET)

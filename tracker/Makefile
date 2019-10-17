BIN = ../bin
CC = gcc
PROG = tracker
FILES = socket
FILES += $(PROG)
OBJ = $(patsubst %,$(BIN)/%.o,$(FILES))

all : $(BIN)/$(PROG)

$(BIN)/%.o : %.c %.h
	$(CC) -c -o $@ $<

$(BIN)/$(PROG) : $(OBJ)
	$(CC) -o $@ $^
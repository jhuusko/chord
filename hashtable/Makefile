BIN = ../bin
CC = gcc
FILES = hash hash_table
OFLAGS = -L/usr/lib -lssl -lcrypto
OBJ = $(patsubst %,$(BIN)/%.o,$(FILES))

all : $(OBJ)

$(BIN)/%.o : %.c %.h
	$(CC) $(OFLAGS) -c -o $@ $<
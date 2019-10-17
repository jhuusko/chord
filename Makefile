##################################################
# Name: Jonas Håkansson, Johan Huusko
# Mail: c15jhn@cs.umu.se, c15jho.cs.umu.se
# Date: 22/9-19
# Project: DoD - P2P distributed hash table.
##################################################
# Usage:
# make            - Installs node and tracker.
# make run        - Runs node.
# make tracker    - Runs tracker.
# make valg       - Runs node with valgrind.
# make clean      - Clears your bin and terminal.
##################################################

BIN = ./bin
HASH = ./hashtable
TRAC = ./tracker

CC = gcc
FLAGS = -std=gnu11 -g -Werror
OFLAGS = -L/usr/lib -lssl -lcrypto
MKDIR = mkdir -p

TRAC_ADDR = 130.239.40.32
TRAC_PORT = 5000

PROG = node
FILES = pdu
FILES += $(PROG)
HASH_FILES = hash hash_table

OBJ = $(patsubst %, $(BIN)/%.o, $(FILES))
_OBJ = $(patsubst %, $(BIN)/%.o, $(HASH_FILES))

all	: $(BIN) mk_tracker mk_hash $(BIN)/$(PROG)

run :
	$(BIN)/$(PROG) $(TRAC_ADDR) $(TRAC_PORT)

tracker :
	$(BIN)/$@ $(TRAC_PORT)

mk_tracker :
	@cd $(TRAC) && $(MAKE) --no-print-directory

mk_hash :
	@cd $(HASH) && $(MAKE) --no-print-directory

$(BIN) :
	$(MKDIR) $(BIN)

$(BIN)/%.o : %.c %.h
	$(CC) -c -o $@ $<

$(BIN)/$(PROG) : $(OBJ)
	$(CC) $(FLAGS) $(OFLAGS) -o $@ $^ $(_OBJ)

valg :
	valgrind --leak-check=yes --show-reachable=yes --track-fds=yes $(BIN)/$(PROG) $(TRAC_ADDR) $(TRAC_PORT)

helg :
	valgrind --tool=helgrind $(BIN)/$(PROG) $(TRAC_ADDR) $(TRAC_PORT)

.PHONY : run tracker clean

clean :
	rm -f $(BIN)/*.o $(shell clear)
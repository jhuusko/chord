#include <stdlib.h>
#include <stdio.h>
#define ADDRESS_LENGTH 16
#include <inttypes.h>
#include <string.h>
#include "list.h"
struct table {
    list* entries;
};

struct entry {
    uint8_t id;
    char address[ADDRESS_LENGTH];
    uint16_t port;
};

int main(void);

struct table* table_init();
bool table_is_empty(const struct table* t);
int table_compare_entries(void*, void*);
void table_sort(struct table*);

void table_add_entry(struct table* t, uint8_t id, char* address, uint16_t port);
void table_print_entry(void*);
void table_free(struct table*);
void table_remove_entry(struct table* t, uint8_t id); 
void table_print(struct table* t);

/*
void free_everything(struct table* entries);
void print_table(struct table* entries);
void remove_entry(struct table* entries, uint8_t id);
void update_entry(struct table* entries, uint8_t id);
struct entry* get_entry(struct table* entries, uint8_t id);
*/

#include "hash_table.h"

/**
 * Written by klasa and kuba for the course 5DV197. Use and modifications
 * are free within the scope of the course. The individual functions are
 * documented in the header file 'hash_table.h'.
 *
 * klasa@cs.umu.se
 * jakub@cs.umu.se
 */

struct hash_table* table_create(hash_function* hash_func, uint32_t max_size) {
    struct hash_table* table = calloc(sizeof(struct hash_table), 1);
    table->max_entries = max_size;
    table->entries = calloc(sizeof(struct table_entry*), max_size);
    table->hash_func = hash_func;
    return table;
}

void table_insert(struct hash_table* table, char* ssn, char* name, char* email) {
    hash_t hash = table->hash_func(ssn) % table->max_entries;
    struct table_entry* entry = calloc(sizeof(struct table_entry), 1), *current, *previous = NULL;
    entry->ssn = strcpy(calloc(1, strlen(ssn) + 1), ssn);
    entry->name = strcpy(calloc(1, strlen(name) + 1), name);
    entry->email = strcpy(calloc(1, strlen(email) + 1), email);

    if(table->entries[hash]) {
        current = table->entries[hash];
        while(current) {
            if(!strcmp(ssn, current->ssn)) {
                entry->next = current->next;
                entry_free(current);
                
                if(previous) {
                    previous->next = entry;
                } else {
                    table->entries[hash] = entry;
                }

                return;
            } 

            previous = current;
            current = current->next;
        }

        previous->next = entry;

    } else {
        table->entries[hash] = entry;
    }
}

void table_remove(struct hash_table* table, char* ssn) {
    hash_t hash = table->hash_func(ssn) % table->max_entries;
    struct table_entry* entry = table->entries[hash], *previous = NULL;

    while(entry) {
        if(!strcmp(entry->ssn, ssn)) {
            if(!previous) {
                table->entries[hash] = entry->next;
            } else {
                previous->next = entry->next;
            }

            entry_free(entry);

            return;
        }

        entry = entry->next;
    }
}

struct table_entry* table_lookup(struct hash_table* table, char* ssn){
    hash_t hash = table->hash_func(ssn) % table->max_entries;
    struct table_entry* entry = table->entries[hash]; 

    while(entry) {
        if(!strcmp(entry->ssn, ssn)) {
            return entry;
        }

        entry = entry->next;
    }

    return NULL;
}

void table_free(struct hash_table* table) {
    for(int i = 0; i < table->max_entries; i++) {
        struct table_entry* entry = table->entries[i], *next; 

        while(entry) {
            next = entry->next; 
            entry_free(entry);
            entry = next;
        }
    }

    free(table->entries);
    free(table);
}

void table_shrink(struct hash_table* table, uint32_t new_table_size) {
    for(int i = new_table_size; i < table->max_entries; i++) {
        struct table_entry* entry = table->entries[i], *next; 
        while(entry) {
            next = entry->next; 
            entry_free(entry);
            entry = next;
        }
    }

    table->entries = realloc(table->entries, sizeof(struct table_entry*) * new_table_size);
    table->max_entries = new_table_size;
}

void table_grow(struct hash_table* table, uint32_t new_table_size) {
    table->entries = realloc(table->entries, sizeof(struct table_entry*) * new_table_size);
    memset(table->entries + table->max_entries, 0, sizeof(struct table_entry*) * (new_table_size - table->max_entries));
    table->max_entries = new_table_size;
}

void entry_free(struct table_entry* e) {
    free(e->name);
    free(e->email);
    free(e->ssn);
    free(e);
}

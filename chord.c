#include "chord.h"

int main(void) {
    struct table* t = table_init();
    table_add_entry(t, 1, "127.0.0.0", 5000);
    table_add_entry(t, 4, "127.0.1.1", 5000);
    table_add_entry(t, 7, "127.1.1.1", 5000);
    table_add_entry(t, 3, "127.0.0.1", 5000);
    table_sort(t); 
    table_print(t);
    printf("------------- \n");
    table_remove_entry(t, 3);
    printf("should have removed entry with id: 3\n");
    list_print(t->entries, table_print_entry);
    table_free(t);
    printf("Bye!\n");
}

//ini table
struct table* table_init() {
   struct table* t = malloc(sizeof(struct table));
   //setting next to null at init
   t->entries = list_empty(NULL);

   return t;
}

//Used to check if table is empty
bool table_is_empty(const struct table* t){
    return is_empty(t->entries);
}
/*
 * Adds an entry to the table, right now the list only supprorts adding an element to the front of the list
 * One workaround is to sort the list each time an element is added
 */
void table_add_entry(struct table* t, uint8_t id, char* address, uint16_t port){
    struct entry* e = malloc(sizeof(struct entry));
    e->id = id;
    memcpy(e->address, address, ADDRESS_LENGTH);
    e->port = port;

    list_insert(t->entries, e);
}
  
/*
 * Gets the entry of the table that is corresponding to the id
 * Returns: the entry if found, NULL if no entry with id was found
 */
struct entry* table_get_entry(struct table* t, uint8_t id){
    
    node* n = list_first(t->entries);

    while(n != NULL){
        struct entry* e = (struct entry*) n->value;

        if(e->id == id){
            return e;
        }
        n = list_next(n);
    }

    return NULL;

}
/*
 * updates the entry in the table corresponding to the id
 *
 */
void table_update_entry(struct table* entries, uint8_t id) {

}

/*
 * Removes an entry in the table correspoinding to the id given to the function
 *
 */ 
void table_remove_entry(struct table* t, uint8_t id) {

    node *n = list_first(t->entries);

    while(n->next != NULL){
        struct entry* e = (struct entry*) n->next->value;

        if(e->id == id){
            list_remove(t->entries, n);
        }
        n = list_next(n);
    }
}

/*
 * frees all dynamically allocated memory associated with the table
 */
void table_free(struct table* t){
    list_kill(t->entries);
    free(t);

}
void table_sort(struct table* t){
    list_sort(t->entries, table_compare_entries);
}
/*
 * Print function used to print the contents of the table
 * The format of the print statement should be changed in table_print_entry
 */
void table_print(struct table* t){
    list_print(t->entries, table_print_entry);
}

/* 
 * Print function used by the list.
 * Should be used in print_table() to print each entry that contains the struct entry
 */
void table_print_entry(void* data){
    
    struct entry* e = (struct entry*) data;

    printf("table_print_entry()\n");
    printf("entry->id: %d \n", e->id);
    printf("entry->address: %s \n", e->address);
    printf("entry->port: %d \n", e->port);

}
/*
 * compare function used to sort the values in the table
 * should return -1 if d1->id < d2->id i think
 * returns 0 if d1->id == d2->id
 *
 */
int table_compare_entries(void* d1, void* d2){
    
    struct entry* e1 = (struct entry*) d1;
    struct entry* e2 = (struct entry*) d2;

    if(e1->id > e2->id){
        return 1;
    }
    else if(e1->id == e2->id){
        return 0;
    }
    else{
        return -1;
    }
}

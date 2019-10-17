#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
/*
 * Typedefs to be used as function pointers
 */
typedef void (*free_function)(void *);
typedef void (*inspect_callback)(void *);
typedef int compare_function(void*, void*);

typedef struct node {
	void* value;
	struct node* next;
} node;

typedef struct list {
	node* head;
	free_function free_func;
} list;

/*
* list_empty
* Creates an empty list
* Parameters: void
* Return: an empty linked list
*/
list* list_empty(free_function);

/*
* is_empty
* Checks if the given list is empty
* Parameters: the list to check
* Return: True if the list is empty, false otherwise
*/
bool is_empty(list* l);

/*
* list_first
* Parameters: the list to check
* Return: the first node in the given list
*/
node* list_first(list* l);

/*
* list_next
* Parameters: any node of the list
* Returns: the node after the given one
*/
node* list_next(node* n);

/*
* list_inspect
* Parameters: the node to inspect
* Return: the value of the node
*/
void* list_inspect(node* n);

/*
* list_insert
* Inserts a new node with a given data to the list
* Parameters: l: the list to add to, d: data to be inserted
* Return: the newly made node
*/
node* list_insert(list* l, void* d);

/*
 * list_remove
 * Removes the given node from the list
 * Parameters: l, the list to remove from, n: the node to remove
 * Returns: a pointer to the next node
 */
node* list_remove(list* l, node* n);

/*
* list_sort
* Sorts the list in rising order using Bubblesort
* Parameters: list to sort
* Return: the sorted list
*/
list* list_sort(list* l, compare_function comp_func);

/*
* list_print
* Prints the data of the list
* Parameters: l: list to print, printFunc: function called for each element
* Return: void
*/
void list_print(list* l, inspect_callback print_func);

/*
* list_kill
* Kills the given list, returning all dynamic memory used.
* Parameters: list to kill
* Return: void
*/
void list_kill(list* l);

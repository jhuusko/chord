#include "list.h"

 
/*
* list_empty
* Creates an empty list
* Parameters: void
* Return: a pointer to the newly created list
*/

list* list_empty(free_function free_func) {

	list* l = malloc(sizeof(list));
	if (l == NULL) {

		perror("Could not create list");
		exit(EXIT_FAILURE);
	}

	l->head = malloc(sizeof(node));

	if (l->head == NULL) {

		perror("Could not create list head");
		exit(EXIT_FAILURE);
	}

	l->head->next = NULL;
	l->free_func = free_func;

	return l;
}

/*
* is_empty
* Checks if the given list is empty
* Parameters: the list to check
* Return: True if the list is empty, false otherwise
*/
bool is_empty(list* l) {

	return(l->head->next == NULL);
}

/*
* list_first
* Parameters: the list to check
* Return: the first node in the given list
*/
node* list_first(list* l) {

	return l->head->next;
}

/*
* list_next
* Parameters: any node of the list
* Returns: the node after the given one
*/
node* list_next(node* n) {

	return n->next;
}

/*
* list_inspect
* Parameters: the node to inspect
* Return: the value of the node
*/
void* list_inspect(node* n) {

	return n->value;
}

/*
* list_insert
* Inserts a new node with a given data to the list
* Parameters: l: the list to add to, d: data to be inserted
* Return: a pointer to the newly created node
*/
node* list_insert(list* l, void* d) {

	node* n = malloc(sizeof(node));

	if(n == NULL) {

		perror("Could not malloc node.");
		exit(EXIT_FAILURE);
	}

	n->value = d;

	n->next = l->head->next;
	l->head->next = n;

	return n;
}
/*
 * list_remove
 * Parameters: l: the list to remove from, n: the node to remove
 * Return: the position of the next node
 */
node* list_remove(list* l, node* n){
    
        node *tmp = n->next;

        n->next = tmp->next;

        if(l->free_func != NULL){
            l->free_func(tmp->value);
        }

        free(tmp);

        return n;
}

/*
* list_sort
* Sorts the list in rising order using Bubblesort
* Parameters: list to sort
* Return: the sorted list
*/
list* list_sort(list* l, compare_function comp_func) {

	if(!is_empty(l)) {

		bool sorted = false;

		if(comp_func == NULL) {
			perror("Cannot sort list without compare function, exiting...");
			exit(EXIT_FAILURE);
		}

		while(!sorted) {

			node* n = list_first(l);
			node* m = list_next(n);
			sorted = true;

			while(m != NULL) {

				if(comp_func(n->value, m->value) > 0) {

					void* tmp = n->value;
					n->value = m->value;
					m->value = tmp;

					sorted = false;
				}

				n = m; 
				m = list_next(n);
			}
		}
	}

	return l;
}

/*
* list_print
* Prints the data of the list
* Parameters: l: list to print, printFunc: function called for each element
* Return: void
*/
void list_print(list* l, inspect_callback print_func) {

	node* n = list_first(l);

	while(n != NULL) {

		print_func(n->value);
		n = list_next(n);
	}
}

/*
* list_kill
* Kills the given list, returning all allocated memory
* Parameters: list to kill
* Return: void
*/
void list_kill(list* l) {

	if(!is_empty(l)) {

		node* n = list_first(l);
		node* tmp;

		while(n != NULL) {

			tmp = n;
			n = list_next(n);

			if(l->free_func != NULL) {

				l->free_func(tmp->value);
			}

			free(tmp->value);
			free(tmp);
		}
	}

	free(l->head);
	free(l);
}

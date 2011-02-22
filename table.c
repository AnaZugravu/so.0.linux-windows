#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include "hash.c"

#define DIE(assertion, call_description)		\
	do {										\
		if (assertion) {						\
			fprintf(stderr, "(%s, %s, %d): ",	\
			__FILE__, __FUNCTION__, __LINE__);	\
			perror(call_description);	\
			exit(EXIT_FAILURE);					\
		}										\
	} while(0)
	
#define RUNT_TEST 0
#define CHUNK_SIZE 255
#define NR_TOKENS 10


typedef struct node {
	char* value;
	struct node* next;
} node;

typedef struct hash_t {
	node** head;
	int length;
} hash_t;

node** new_buckets(int hash_length) 
{
	int i;
	node** buckets = malloc(sizeof(node*) * hash_length);
	if (buckets == NULL) {
		return NULL;
	}
	
	for (i = 0; i < hash_length; i++)
		buckets[i] = NULL;
	return buckets;
}


hash_t* new_hash(int hash_length)
{
	hash_t* hash_table = malloc(sizeof(hash_t));
	if (hash_table == NULL)
		return NULL;
		
	hash_table->length = hash_length;
	hash_table->head = new_buckets(hash_length);
	if (hash_table->head == NULL) {
		free(hash_table);
		return NULL;
	}
	
	return hash_table;
}

node* find(char *search_term, hash_t* hash_table)
{
	int key = hash(search_term, hash_table->length);
	node* np = hash_table->head[key];
	for(; np != NULL; np = np->next)
		if (strcmp(np->value, search_term) == 0)
			return np;
	return NULL;
}

int add(char* value_to_add, hash_t* hash_table)
{
	int key;
		
	/*Evitare dubluri*/
	node* new_node = find(value_to_add, hash_table);
	if (new_node != NULL)
		return -1;
	
	key = hash(value_to_add, hash_table->length);
	new_node = malloc(sizeof(node));
	if(new_node == NULL)
		return 0;
	
	int str_len = strlen(value_to_add) + 1;
	new_node->value = malloc(str_len);
	if(new_node->value == NULL) {
		free(new_node);
		return 0;
	}
	memcpy(new_node->value, value_to_add, str_len);
	
	node* it = hash_table->head[key];
	if (it == NULL) {
		hash_table->head[key] = new_node;
		hash_table->head[key]->next = NULL;
		return 1;
	}
	
	for (; it->next; it = it->next);
	it->next = new_node;
	
	return 1;
}

void rem(char* value, hash_t* hash_table)
{
	int key;
		
	node* np = find(value, hash_table);
	if (NULL == np)
		return;
		
	key = hash(value, hash_table->length);
	
	if (hash_table->head[key] == np) {
		hash_table->head[key] = np->next;
		free(np->value);
		free(np);
		return;
	}
	node* it = hash_table->head[key];
	for (; it->next != np; it = it->next);
	it->next = np->next;
	free(np->value);
	free(np);
	return;
}

void clear(hash_t* hash_table)
{
	int i;
		
	for (i = 0; i < hash_table->length; i++)
		if (hash_table->head[i] != NULL) {
			node* np = hash_table->head[i];
			for (; np != NULL; np = np->next) {
				node* aux = np;
				free(aux->value);
				free(aux);
			}
			hash_table->head[i] = NULL;
		}
}


void free_hash(hash_t* hash_table)
{
	clear(hash_table);
	free(hash_table->head);
	free(hash_table);
}

void print_bucket(int key, FILE* f, hash_t* hash_table)
{
	node* np = hash_table->head[key];
	/*fprintf(f, "\t[%d] - ", key);*/
	for(; np != NULL; np = np->next)
			fprintf(f, "%s ", np->value);
	fprintf(f, "\n");
}

void print_hash(FILE* f, hash_t* hash_table)
{
	int i;
	for (i = 0; i < hash_table->length; i++)
		if (hash_table->head[i] != NULL)
			print_bucket(i, f, hash_table);
}

void swap_hash_tables(hash_t* h1, hash_t* h2)
{
	hash_t aux;
	aux = *h1;
	*h1 = *h2;
	*h2 = aux;
}

void resize(hash_t* hash_table, int new_length)
{
	int i;
	hash_t* new_hash_table = new_hash(new_length);
	
	for (i = 0; i < hash_table->length; i++) {
		node* np = hash_table->head[i];
 		for (; np != NULL; np = np->next)
 			add(np->value, new_hash_table);
 	}
 	swap_hash_tables(hash_table, new_hash_table);
 	free_hash(new_hash_table);
}

void resize_double(hash_t* hash_table)
{
	resize(hash_table, 2 * hash_table->length);
}

void resize_halve(hash_t* hash_table)
{
	resize(hash_table, hash_table->length/2);
}


void run_commands(char* command, char** args, hash_t* hash_table)
{
	
	if (!strcmp(command, "add")) {
		add(args[0], hash_table);
		return;
	}
	
	if (!strcmp(command, "remove")) {
		rem(args[0], hash_table);
		return;
	}
	
	if (!strcmp(command, "clear")) {
		clear(hash_table);
		return;
	}
	
	if (!strcmp(command, "find")) {
		FILE* f = fopen(args[1], "a");
		if (f == NULL)
			f = stdout;
		
		node* np = find(args[0], hash_table);
		if (np)
			fprintf(f, "True\n");
		else
			fprintf(f, "False\n");
		if (f != stdout)
			fclose(f);
		return;
	}
	
	if (!strcmp(command, "print_bucket")) {
		FILE* f = fopen(args[1], "a");
		if (f == NULL) {
			f = stdout;
		}
		print_bucket(atoi(args[0]), f, hash_table);
		if (f != stdout)
			fclose(f);
		return;
	}
	
	if (!strcmp(command, "print")) {
		FILE* f = fopen(args[0], "a");
		if (f == NULL) {
			f = stdout;
		}
		print_hash(f, hash_table);
		if (f != stdout)
			fclose(f);
	}
	
	if (!strcmp(command, "resize double")) {
		resize_double(hash_table);
		return;
	}
	
	
	if (!strcmp(command, "resize halve")) {
		resize_halve(hash_table);
		return;
	}
}


void parse_line(char* line, hash_t* hash_table)
{
	char* word = malloc(CHUNK_SIZE);
	word = strtok(line, " ");
	if (word == NULL) {
		printf("EMPTY LINE.\n");
		return;
	}
	char* command = malloc(strlen(word));
	memcpy(command, word, strlen(word));
	
	
	if (!strcmp(command, "resize")) {
		word = strtok(NULL, " ");
		char* first_part = malloc(strlen(command));
		memcpy(first_part, command, strlen(command));
		
		command = realloc(command, strlen(command) + strlen(word) + 1);
		memcpy(command, first_part, strlen(first_part));
		memcpy(command + strlen(first_part), " ", 1);
		strncat(command, word, strlen(word));
		run_commands(command, NULL, hash_table);
		
		return;
	}
	
	int nr_args = 0;
	char** args = malloc(nr_args * sizeof(char*));
	while(word) {
		word = strtok(NULL, " ");
		if (word) {
			nr_args++;
			args = realloc(args, nr_args * sizeof(char*));
			args[nr_args - 1] = malloc(strlen(word));
			memcpy(args[nr_args - 1], word, strlen(word));
		}
	}
	run_commands(command, args, hash_table);
	
}


void read_files(int argc, char** argv, hash_t* hash_table)
{
	int i;
	int nr_files = argc - 2;
	char* command = malloc(CHUNK_SIZE + 1);
	for (i = 0; i < nr_files; i++) {
		FILE* f = fopen(argv[i + 2], "r");
		while (!feof(f)) {
			fgets(command, CHUNK_SIZE, f);
			if (command[strlen(command)-1] == '\n' || command[strlen(command)-1] == '\r')
				command[strlen(command)-1] = '\0';
			/*if (strchr(command, '\n'))*/
				parse_line(command, hash_table);
		}
		fclose(f);
	}
}

void read_stdin()
{
	printf("READ STDIN\n");
}

void read_input(int argc, char** argv, hash_t* hash_table)
{
	if (argc == 2)
		read_stdin();
	else
		read_files(argc, argv, hash_table);
}

int main(int argc, char** argv)
{
	int hash_length = 0;
	if (argc < 2) {
		fprintf(stderr, "Too few arguments.\n");
		exit(EXIT_FAILURE);
	}
		
	hash_length = atoi(argv[1]);
	hash_t* hash_table = new_hash(hash_length);
	
	read_input(argc, argv, hash_table);
	free(hash_table);
	
	return 0;
}



/*void test(hash_t* hash_table)
{
	add("mimi", hash_table);
	print_hash(NULL, hash_table);
	DIE(!find("mimi", hash_table), "NOT FOUND");
	
	add("mama", hash_table);
	print_hash(NULL, hash_table);
	DIE(!find("mama", hash_table), "NOT FOUND");
	
	add("riri", hash_table);
	print_hash(NULL, hash_table);
	DIE(!find("riri", hash_table), "NOT FOUND");
	
	add("fofo", hash_table);
	print_hash(NULL, hash_table);
	DIE(!find("fofo", hash_table), "NOT FOUND");
	
	add("sasa", hash_table);
	print_hash(NULL, hash_table);
	DIE(!find("sasa", hash_table), "NOT FOUND");
	
	
	add("jhadsfladskf", hash_table);
	print_hash(NULL, hash_table);
	DIE(!find("sasa", hash_table), "NOT FOUND");
	
	add("ywue", hash_table);
	print_hash(NULL, hash_table);
	DIE(!find("ywue", hash_table), "NOT FOUND");
	
	
	rem("ywue", hash_table);
	print_hash(NULL, hash_table);
	DIE(find("ywue", hash_table), "FOUND INEXISTING");
	
	rem("fofo", hash_table);
	print_hash(NULL, hash_table);
	DIE(find("fofo", hash_table), "FOUND INEXISTING");
	
	rem("riri", hash_table);
	print_hash(NULL, hash_table);
	DIE(find("riri", hash_table), "FOUND INEXISTING");
	
	
	rem("mimi", hash_table);
	print_hash(NULL, hash_table);
	DIE(find("mimi", hash_table), "FOUND INEXISTING");
	
	printf("DOUBLE:\n");
	resize_double(hash_table);
	print_hash(NULL, hash_table);
}
*/



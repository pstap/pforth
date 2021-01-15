#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* STRUCTURE DEFINITIONS */
/* Linked list implementation of a dict */
struct dict_node {
    size_t label_size;
    char* label;
    void (*fptr)(void);
    struct dict_node* next;
};

struct dict {
    struct dict_node* head;
};

struct stack {
    int64_t size;
    int64_t* base;
    int64_t* top;
};

/* Globals */
/* Global dictionary */
struct dict DICT = {NULL};

struct stack STACK = {0, NULL};

/* Change all these names */
size_t BUFFER_SIZE = 256;
char* INPUT_BUFFER = NULL;
char* END_INPUT_BUFFER = NULL;
char* INPUT_POS = NULL;
size_t INPUT_SIZE = 0;
size_t PROCESSED_SIZE = 0;
char* CURRENT_WORD = NULL;
size_t WORD_SIZE = 0;


/* Dict API */
struct dict_node* new_dict_node(char* label, void (*fptr)(void)) {
    struct dict_node* temp = malloc(sizeof(struct dict_node));
    const size_t label_size = strnlen(label, BUFFER_SIZE);
    temp->label = malloc(label_size + 1 ); // +1 for \0
    temp->label_size = label_size;
    strncpy(temp->label, label, label_size);
    temp->label[label_size] = '\0';
    temp->next = NULL;
    temp->fptr = fptr;
    return temp;
}

void free_dict_node(struct dict_node* n) {
    free(n->label);
    free(n);
}

void free_dictionary() {
    struct dict_node* n = DICT.head;
    struct dict_node* next = NULL;
    while(n != NULL) {
        next = n->next;
        free_dict_node(n);
        n = next;
    }
}

struct dict_node* add_dict_node(char* label, void (*fptr)(void)) {
    struct dict_node* n = DICT.head;

    // Ugly edge case
    if(n == NULL) {
        DICT.head = new_dict_node(label, fptr);
        return n;
    }
    while(n->next != NULL) {
        n = n->next;
    }
    n->next = new_dict_node(label, fptr);
    return n;
}

struct dict_node* find_in_dict(char* word) {
    struct dict_node* n = DICT.head;
    while(1) {
        if(n == NULL) {
            return NULL;
        }
        if(strncmp(word, n->label, strlen(word)) == 0) {
            return n;
        } else {
            n = n->next;
        }
    }
    return n;
}

void print_dict_node(struct dict_node* n) {
    printf("\"%s\" (%ld)\t%p\n", n->label, n->label_size, n->fptr);
}

void print_dict(struct dict_node* n) {
    int i = 0;
    while(n != NULL) {
        fprintf(stdout, "%d\t", i);
        print_dict_node(n);
        ++i;
        n = n->next;
    }
}

/* STACK API */
void stack_push(int64_t d) {
    STACK.top[STACK.size++] = d;
}

int64_t stack_pop(void) {
    if(STACK.size == 0) {
        fprintf(stderr, "ERROR: stack underflow\n");
        return 0;
    }
    return STACK.top[--STACK.size];
}

/* TODO ALL OF THESE SHOULD HANDLE ERROR CASES LIKE STACK UNDERFLOW */
void OP_ADD(void) {
    const int64_t y = stack_pop();
    const int64_t x = stack_pop();
    stack_push(x + y);
}

void OP_SUB(void) {
    const int64_t y = stack_pop();
    const int64_t x = stack_pop();
    stack_push(x - y);
}

void OP_MUL(void) {
    const int64_t y = stack_pop();
    const int64_t x = stack_pop();
    stack_push(x * y);
}

void OP_DUP(void) {
    const int64_t x = STACK.top[STACK.size - 1];
    stack_push(x);
}

void OP_SWAP(void) {
    if(STACK.size < 2) {
        fprintf(stderr, "ERROR: stack underflow");
    }
    const int64_t temp = STACK.top[STACK.size - 1];
    STACK.top[STACK.size - 1] = STACK.top[STACK.size - 2];
    STACK.top[STACK.size - 2] = temp;
}

void OP_POP_PRINT(void) {
    const int64_t p = stack_pop();
    fprintf(stdout, "%ld", p);
}

void OP_CR(void) {
    fprintf(stdout, "\n");
}

void META_DICT(void) {
    print_dict(DICT.head);
}

void META_WORDS(void) {
    struct dict_node* n = DICT.head;
    fprintf(stdout, "[");
    while(n != NULL) {
        if(n->next == NULL) {
            fprintf(stdout, "%s", n->label);
            break;
        } else {
            fprintf(stdout, "%s, ", n->label);
            n = n->next;
        }
    }
    fprintf(stdout, "]\n");
}

void META_STACK(void) {
    fprintf(stdout, "( ");
    for(int i = 0; i < STACK.size; ++i) {
        fprintf(stdout, "%ld ", (int64_t) STACK.top[i]);
    }
    fprintf(stdout, ") (%ld)\n", STACK.size);
}

/* Initialize buffers and sizes */
bool init()
{
    INPUT_BUFFER = malloc(BUFFER_SIZE);
    if(INPUT_BUFFER == NULL) {
        fprintf(stderr, "Could not initialize input buffer\n");
        return false;
    }
    END_INPUT_BUFFER = INPUT_BUFFER + BUFFER_SIZE;
    INPUT_POS = INPUT_BUFFER;

    CURRENT_WORD = malloc(BUFFER_SIZE);
    if(CURRENT_WORD == NULL) {
        fprintf(stderr, "Could not initialize WORD buffer\n");
        return false;
    }

    STACK.base = malloc(sizeof(int64_t) * 4096);
    STACK.top = STACK.base;
    if(STACK.base == NULL) {
        fprintf(stderr, "Could not initialize STACK\n");
        return false;
    }

    /* add primary words to dict */
    add_dict_node("+", &OP_ADD);
    add_dict_node("-", &OP_SUB);
    add_dict_node("*", &OP_MUL);
    add_dict_node(".", &OP_POP_PRINT);
    add_dict_node("CR", &OP_CR);
    add_dict_node("DUP", &OP_DUP);
    add_dict_node("SWAP", &OP_SWAP);
    add_dict_node("DICT", &META_DICT);
    add_dict_node("WORDS", &META_WORDS);
    add_dict_node(".s", &META_STACK);
    return true;
}


/* Naming is terrible*/
/* Status returned by next_word() */
typedef enum input_status {
    STATUS_WORD_READ = 0,
    STATUS_MORE = 1,
    STATUS_EMPTY = 2
} input_status_e;

/* Try to read the next word into the buffer */
input_status_e next_word() {
    WORD_SIZE = 0;

    if(INPUT_SIZE == 0 || INPUT_SIZE == -1) {
        return STATUS_EMPTY;
    }

    // eat leading whitespace
    while(isspace(*INPUT_POS) && (INPUT_POS != END_INPUT_BUFFER)) {
        ++INPUT_POS;
        ++PROCESSED_SIZE;
    }

    // Are we at the end of the input?
    if((INPUT_POS == END_INPUT_BUFFER) || (PROCESSED_SIZE == INPUT_SIZE)) {
        return STATUS_MORE;
    }

    char* p = CURRENT_WORD;
    while(!isspace(*INPUT_POS)) {
        *p++ = *INPUT_POS++;
        ++WORD_SIZE;
        ++PROCESSED_SIZE;
    }
    *p = '\0';

    return STATUS_WORD_READ;
}

void get_input_from_stdin()
{
    INPUT_POS = INPUT_BUFFER;
    INPUT_SIZE = getline(&INPUT_BUFFER, &BUFFER_SIZE, stdin);
    PROCESSED_SIZE = 0;
}

void cleanup() {
    free_dictionary();
    free(STACK.top);
    free(INPUT_BUFFER);
    free(CURRENT_WORD);
}

int main(int argc, char* argv[])
{
    const bool init_success = init();
    input_status_e status;
    bool run = true;

    if(!init_success)
    {
        fprintf(stderr, "INIT failed\n");
        return -1;
    }

    fprintf(stdout, "Welcome to pforth\n");
    fprintf(stdout, "Type DICT<ENTER> for a DICT listing, or WORDS<ENTER> for all words\n");
    fprintf(stdout, "\n");

    while(run) {
        fprintf(stdout, "> ");
        get_input_from_stdin();

        while(1) {
            status = next_word();
            if(status == STATUS_MORE) {
                break;
            } else if(status == STATUS_EMPTY)  {
                run = false;
                break;
            }
            struct dict_node* n = find_in_dict(CURRENT_WORD);
            if(n == NULL) {
                /* Try pushing it to the stack */
                stack_push((int64_t)atoi(CURRENT_WORD));
            } else {
                n->fptr();
            }
        }
    }

    cleanup();

    fprintf(stdout, "bye.\n");
    return 0;
}

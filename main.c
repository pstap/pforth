#define _GNU_SOURCE

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* UTILITY */
void copy_string_safe(char* dest, const char* src, size_t max_size) {
    /* TODO This doesn't insert null terminator - probably doesn't work without debug mode */
    for(size_t i = 0; i < max_size; ++i) {
        dest[i] = src[i];
        if(src[i] == '\0')
            break;
    }
}
int string_length_safe(const char* s, size_t max_size) {
    for(size_t i = 0; i < max_size; ++i) {
        if(s[i] == '\0')
            return (int)i;
    }
    return -1;
}

/* LABELS AND LABEL ALLOCATOR */
#define MAX_LABEL_SIZE 256

#define LABEL_MEM_SIZE 1024
char* LABEL_MEM = NULL;

struct label_allocator {
    char* base;
    char* current;
    size_t bytes_allocd;
    size_t labels_allocd;
    size_t total_size;
};

void init_label_allocator(struct label_allocator* a,
                          char* base,
                          size_t total_size) {
    a->base = base;
    a->current = base;
    a->bytes_allocd = 0;
    a->labels_allocd = 0;
    a->total_size = total_size;
}

struct label_allocator LABEL_ALLOC;

char* alloc_label(struct label_allocator* a, char* label) {
    const size_t label_size = string_length_safe(label, MAX_LABEL_SIZE);
    const size_t alloc_size = label_size + 1; // Total size with null terminator
    // Out of space
    if((a->bytes_allocd + alloc_size) > a->total_size) {
        return NULL;
    }
    char* label_start = a->current;

    copy_string_safe(a->current, label, MAX_LABEL_SIZE);
    a->bytes_allocd += alloc_size;
    a->current += alloc_size;
    ++a->labels_allocd;

    return label_start;
}

/* DICTIONARY */
/* Should be `word` */
struct dict_entry {
    size_t label_size;
    char* label;
    void (*fptr)(void);
};

#define MAX_DICT_ENTRIES 1024
struct dict_entry* DICT;

struct dict_allocator {
    struct dict_entry* start;
    size_t current_size;
    size_t max_size;
};

struct dict_allocator DICT_ALLOC;

struct dict_entry* alloc_dict_entry(struct dict_allocator* da,
                                    struct label_allocator* la,
                                    char* label,
                                    void (*fptr)(void)) {
    // allocation failure, can't allocate any more
    if(da->current_size == da->max_size) {
        return NULL;
    }
    struct dict_entry* current = &da->start[da->current_size];
    char* new_label = alloc_label(la, label);
    // label allocation failure
    if(new_label == NULL) {
        return NULL;
    }
    current->label = new_label;
    current->label_size = string_length_safe(current->label, MAX_LABEL_SIZE);
    current->fptr = fptr;

    ++da->current_size;

    return current;
}

void init_dict_allocator(struct dict_allocator* a,
                         struct dict_entry* start,
                         size_t max_size) {
    a->start = start;
    a->current_size = 0;
    a->max_size = max_size;
}

/* RUNTIME */



/* STRUCTURE DEFINITIONS */

struct stack {
    int64_t size;
    int64_t* base;
    int64_t* top;
};

/* Globals */
/* Global dictionary */

struct stack STACK = {0, NULL, NULL};

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
struct dict_entry* find_in_dict(struct dict_allocator* a, char* word) {
    const size_t word_len = string_length_safe(word, MAX_LABEL_SIZE);
    for(size_t i = 0; i < a->current_size; ++i) {
        if(word_len != DICT[i].label_size) {
            continue;
        } else {
            const int result = strcmp(DICT[i].label, word);
            if(result == 0) {
                return &DICT[i];
            }
        }
    }
    return NULL;
}

void print_dict_node(struct dict_entry* e) {
    printf("%p\t\"%s\"\n", (void*)e, e->label);
}

void print_dict(struct dict_allocator* a) {
    fprintf(stdout, "i\tAddr\t\tWord\n");
    for(size_t i = 0; i < a->current_size; ++i) {
        fprintf(stdout, "%ld\t", i);
        print_dict_node(&a->start[i]);
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

/* Runtime state */
enum pforth_mode {
    MODE_EXECUTE = 0,
    MODE_COMPILE = 1
};

struct pforth_state {
    enum pforth_mode mode;
};

struct pforth_state STATE = {MODE_EXECUTE};

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
    print_dict(&DICT_ALLOC);
}

void META_WORDS(void) {
    struct dict_entry* n = DICT_ALLOC.start;
    fprintf(stdout, "[");
    for(size_t i = 0; i < DICT_ALLOC.current_size; ++i) {
        if(i == (DICT_ALLOC.current_size - 1)) {
            fprintf(stdout, "%s", n[i].label);
        } else {
            fprintf(stdout, "%s, ", n[i].label);
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

void META_FORTH(void) {
    fprintf(stdout, "STATE: %d\n", STATE.mode);
}

void OP_COMPILE(void) {
    STATE.mode = MODE_COMPILE;
}

void OP_END_COMPILE(void) {
    STATE.mode = MODE_EXECUTE;
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

    // 4K of label memory
    LABEL_MEM = malloc(sizeof(char) * LABEL_MEM_SIZE);
    DICT = malloc(sizeof(struct dict_entry) * MAX_DICT_ENTRIES);

    init_dict_allocator(&DICT_ALLOC, DICT, MAX_DICT_ENTRIES);
    init_label_allocator(&LABEL_ALLOC, LABEL_MEM, LABEL_MEM_SIZE);

    /* Completely unnecessary macro */
#define DICT_ENTRY(WORD,FUNC) alloc_dict_entry(&DICT_ALLOC, &LABEL_ALLOC, WORD, &FUNC)

    /* add primary words to dict */
    DICT_ENTRY(":", OP_COMPILE);
    DICT_ENTRY(";", OP_END_COMPILE);
    DICT_ENTRY("+", OP_ADD);
    DICT_ENTRY("-", OP_SUB);
    DICT_ENTRY("*", OP_MUL);
    DICT_ENTRY(".", OP_POP_PRINT);
    DICT_ENTRY("CR", OP_CR);
    DICT_ENTRY("DUP", OP_DUP);
    DICT_ENTRY("SWAP", OP_SWAP);
    DICT_ENTRY("DICT", META_DICT);
    DICT_ENTRY("WORDS", META_WORDS);
    DICT_ENTRY(".s", META_STACK);
    DICT_ENTRY("FORTH", META_FORTH);

#undef DICT_ENTRY
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

    if(INPUT_SIZE == 0 || INPUT_SIZE == -1UL) {
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
    free(LABEL_MEM);
    free(DICT);
    free(STACK.top);
    free(INPUT_BUFFER);
    free(CURRENT_WORD);
}



int main(void)
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
        /* Interactive */
        fprintf(stdout, "> ");
        get_input_from_stdin();

        /* Process input */
        while(1) {
            status = next_word();
            /* HERE WE CHANGE BEHAVIOUR BASED ON COMPILE VS NON-COMPILE MODE */
            if(status == STATUS_MORE) {
                break;
            } else if(status == STATUS_EMPTY)  {
                run = false;
                break;
            }
            struct dict_entry* w = find_in_dict(&DICT_ALLOC, CURRENT_WORD);
            if(w == NULL) {
                /* Try pushing it to the stack */
                stack_push((int64_t)atoi(CURRENT_WORD));
            } else {
                w->fptr();
            }
        }
    }

    cleanup();

    fprintf(stdout, "bye.\n");
    return 0;
}

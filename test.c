#include <stdio.h>
#include <stdlib.h>

/* ARG STACK */
int *arg_stack = NULL;
int arg_stack_size = 0;

#define ARG_PUSH(X) arg_stack[arg_stack_size++] = X
#define ARG_POP() arg_stack[--arg_stack_size]
#define ARG_STACK_TOP() arg_stack[arg_stack_size - 1]

/* RETURN STACK */
void (***ret_stack)(void) = NULL;
int ret_stack_size = 0;

#define RET_PUSH(X) ret_stack[ret_stack_size++] = X
#define RET_POP() ret_stack[--ret_stack_size]
#define RET_STACK_TOP() ret_stack[ret_stack_size - 1]

void print_arg_stack() {
    for(int i = 0; i < arg_stack_size; ++i) {
        printf("%d - %d\n", i, arg_stack[i]);
    }
}

void print_ret_stack() {
    for(int i = 0; i < ret_stack_size; ++i) {
        printf("%d - %p\n", i, (void*)ret_stack[i]);
    }
}

struct word {
    int inst_count;
    // Pointer to array of function pointers
    void (**code)(void);
};

struct word dup_word;
struct word mul_word;
struct word sqr_word;
struct word quad_word;


struct state {
    void (*w)(void);
    void (**ip)(void);
};
struct state STATE;

#define NEXT() do { \
    STATE.w = *STATE.ip++; \
    STATE.w(); \
} while(0)

void docol() {
    RET_PUSH((STATE.ip + 1));
    STATE.ip = (void (**)(void))(*STATE.ip);
    NEXT();
}

void ret() {
    if(ret_stack_size == 0)
        return;
    else {
        STATE.ip = RET_POP();
        NEXT();
    }
}


void dup_imp(void) {
    const int x = ARG_STACK_TOP();
    ARG_PUSH(x);
    NEXT();
}

void mul_imp(void) {
    const int y = ARG_POP();
    ARG_STACK_TOP() *= y;
    NEXT();
}

void init() {
#define CODE_ALLOC(COUNT) malloc(sizeof(void (**)(void)) * COUNT)
    arg_stack = malloc(sizeof(int) * 4096);
    ret_stack = malloc(sizeof(void (**)(void)) * 4096);

    // DUP
    dup_word.inst_count = 1;
    dup_word.code = CODE_ALLOC(1);
    dup_word.code[0] = &dup_imp;

    // MUL
    mul_word.inst_count = 1;
    mul_word.code = CODE_ALLOC(1);
    mul_word.code[0] = &mul_imp;

    // SQR
    sqr_word.inst_count = 3;
    sqr_word.code = CODE_ALLOC(3);
    sqr_word.code[0] = *dup_word.code;
    sqr_word.code[1] = *mul_word.code;
    sqr_word.code[2] = &ret;

    // QUAD
    quad_word.inst_count = 5;
    quad_word.code = CODE_ALLOC(5);
    quad_word.code[0] = &docol;
    quad_word.code[1] = (void (*)(void)) sqr_word.code;
    quad_word.code[2] = &docol;
    quad_word.code[3] = (void (*)(void)) sqr_word.code;
    quad_word.code[4] = &ret;

}

void execute_word(struct word* w) {
    STATE.ip = w->code;
    NEXT();
}

int main() {
    init();

    ARG_PUSH(5);
    execute_word(&sqr_word);
    print_arg_stack();

    printf("----------\n");

    ARG_PUSH(10);
    execute_word(&quad_word);

    print_arg_stack();


    free(arg_stack);
    return 0;
}

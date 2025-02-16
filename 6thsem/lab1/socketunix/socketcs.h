#ifndef SOCKETCS_H__
#define SOCKETCS_H__

#define ADD '+'
#define SUB '-'
#define MUL '*'
#define DIV '/'

#define ZERO_DIVISION_ERROR 1
#define INVALID_OPERATION_ERROR 2
#define INPUT_ERROR 3
#define OK 0

#define SOCKET_NAME "calc.socket"

struct calculate_in {
    double a, b;
    char operation;
};

typedef struct calculate_in calculate_in;

struct calculate_out {
    double result;
    int error;
};

typedef struct calculate_out calculate_out;



#endif
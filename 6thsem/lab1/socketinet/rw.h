#ifndef RW_H__
#define RW_H__

#define OK 0
#define WRITE_BUSY_ERROR 1
#define INVALID_OPERATION_ERROR 2
#define INVALID_FORMAT_ERROR 3
#define OUT_OF_RANGE_ERROR 4

#define PORT 3233

#define READ 0
#define WRITE 1

#define MAX_MESSAGE_SIZE 4096
#define ARRAY_SIZE 64
#define ARRAY_ELEMENT_BUSY '*'
#define ARRAY_ELEMENT_FREE '_'

struct rw_in {
    int type;
    int index;
};

typedef struct rw_in rw_in;

struct r_out {
    char arr[ARRAY_SIZE];
    int error;
};

struct w_out {
    char result;
    int error;
};

struct err_out {
    int error;
};

#endif


#ifndef RW_H__
#define RW_H__

#define OK 0
#define WRITE_BUSY_ERROR 1
#define INVALID_OPERATION_ERROR 2
#define INVALID_FORMAT_ERROR 3

#define PORT 3232

#define READ 0
#define WRITE 1

#define ARRAY_SIZE 1024
#define ARRAY_ELEMENT_BUSY '*'
#define ARRAY_ELEMENT_FREE '_'

struct rw_in {
    int type;
    int index;
};

typedef struct rw_in rw_in;

struct r_out {
    char arr[ARRAY_SIZE];
};

struct w_out {
    int result;
};


struct rw_out
{
    union{
        struct r_out r_out;
        struct w_out w_out;
    };
    int error;
};

typedef struct rw_out rw_out;
#endif


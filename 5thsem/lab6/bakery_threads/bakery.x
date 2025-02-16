/*
 * filename: calculator.x
     * function: Define constants, non-standard data types and the calling process in remote calls
 */

const ADD = 0;
const SUB = 1;
const MUL = 2;
const DIV = 3;

struct BAKERY_CALCULATOR
{
    int index;
    int pid;
    int num;
    int op;
    float arg1;
    float arg2;
    float result;
};

program BAKERY_PROG
{
    version BAKERY_VER
    {
        struct BAKERY_CALCULATOR get_number(struct BAKERY_CALCULATOR) = 1;
        struct BAKERY_CALCULATOR perform(struct BAKERY_CALCULATOR) = 2;
    } = 1; /* Version number = 1 */
} = 0x20000001; /* RPC program number */

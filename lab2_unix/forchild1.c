#include <stdio.h>
#include <unistd.h>

#define OK 0
#define INPUT_ERROR 1

int main(void)
{
    int a, m;
    printf("PID %d:Enter three-digit non-negative number: ", getpid());
    scanf("%d", &a);
    if (a < 0)
    {
        printf("PID %d:Error: can't be negative\n", getpid());
        return INPUT_ERROR;
    }
    if (a > 999 || a < 100) 
    {
        printf("PID %d:Error: number must be between 100 and 999 inclusive\n", getpid());
        return INPUT_ERROR;
    }
    m = (a % 10) * (a / 10 % 10) * (a / 100 % 10);
    printf("PID %d:Multiplication of num's digits: %d\n", getpid(), m);
    return OK;
}

#include <stdio.h>
#include <math.h>
#include <unistd.h>

#define OK 0
#define IOERROR 1

/**
 * Функция для вычисления s(x) с точностью eps
 */
double s(double x, double eps) 
{
    double now, sum = 0;
    int i = 1;
    now = 1;
    
    while (now >= eps) 
    {
        sum += now;
        now = now * (2 * i - 1) * x * x / (2 * i);
        i++;
    }
    return sum;
}

/**
 * Функция f(x)
 */
double f(double x) 
{ 
    return 1 / sqrt(1 - x * x);
}

/**
 * Функция для вычисления абсолютной погрешности
 */
double abs_error(double s, double f)
{
    return fabs(s - f);
}

/**
 * Функция для вычисления относительной погрешности
 */
double eps_error(double s, double f)
{
    return abs_error(s, f) / fabs(f);
}


int main(void)
{
    double eps, x;
    int rc;
    printf("PID %d: Enter x(-1<=x<=1): ", getpid());
    rc = scanf("%lf", &x);
    if (rc != 1) 
    {
        printf("PID %d: Error: Wrong input.\n", getpid());
        return IOERROR;
    }
    
    if (x <= -1 || x >= 1) 
    {
        printf("PID %d: Error: Wrong input.\n", getpid());
        return IOERROR;
    }


    printf("PID %d: Enter eps: ", getpid());
    rc = scanf("%lf", &eps);
    if (rc != 1) 
    {
        printf("PID %d: Error: Wrong input.\n", getpid());
        return IOERROR;
    }

    if (eps <= 0 || eps > 1)
    {
        printf("PID %d: Error: Wrong input.\n", getpid());
        return IOERROR;
    }


    double fa = f(x), sa = s(x, eps);
    printf("PID %d: s = %lf  f = %lf ", getpid(), sa, fa);
    printf("PID %d: abs = %lf rel = %lf\n", getpid(), abs_error(sa, fa), eps_error(sa, fa));
    return OK;
}

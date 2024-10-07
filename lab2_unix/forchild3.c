#include <stdio.h>
#include <math.h>
#include <unistd.h>

#define OK 0
#define IO_ERROR 1
#define SIZE_ERROR 2
#define EMPTY_ARRAY_ERROR 3
#define NO_POS_NUMS_ERROR 4
#define MAX_SIZE 10

int input(int *array, size_t *array_size);
int pos_geometric_mean(const int *array, size_t array_size, double *res);
double n_root(int value, unsigned int n);
int composition(const int *array, size_t array_size);
size_t pos_num(const int *array, size_t array_size);


/**
* Функция для ввода массива
*/
int input(int *array, size_t *array_size)
{
    int rc;
    printf("PID %d: Enter array size: ", getpid());
    rc = scanf("%zu", array_size);
    if (rc != 1)
    {
        return IO_ERROR;
    }
    if (*array_size == 0)
    {
        return EMPTY_ARRAY_ERROR;
    }
    if (*array_size > MAX_SIZE)
    {
        return SIZE_ERROR;
    }
    for (size_t i = 0; i < *array_size; ++i)
    {
        printf("PID %d: Enter array element: ", getpid());
        rc = scanf("%d", &array[i]);
        if (rc != 1)
        {
            return IO_ERROR;
        }
    }
    return OK;
}

/**
* Функция для подсчета количества положительных элемента массива
*/
size_t pos_num(const int *array, size_t array_size)
{
    size_t num = 0;
    for (size_t i = 0; i < array_size; i++)
    {
        if (array[i] > 0)
        {
            num++;
        }
    }
    return num;
}

/**
* Функция для вычисления корня n-ой степени
*/
double n_root(int value, unsigned int n)
{
    return pow((double)value, 1.0 / n);
}


/**
* Функция для вычисления произведения положительных чисел массива
*/
//int pos_composition(const int *array, size_t array_size)
//{
//    int comp = 1;
//    for (size_t i = 0; i < array_size; ++i)
//    {
//        if (array[i] > 0)
//        {
//            comp *= array[i];
//        }
//    }
//    return comp;
//}

/**
* Функция для вычисления среднего геометрического положительных элементов массива
*/
//int pos_geometric_mean(const int *array, size_t array_size, double *res)
//{
//    unsigned int pos_n = pos_num(array, array_size);
//    if (pos_n == 0)
//    {
//        return NO_POS_NUMS_ERROR;
//    }
//    *res = n_root(pos_composition(array, array_size), pos_n);
//    return OK;
//}

/**
* Функция для вычисления среднего геометрического положительных элементов массива за один цикл
*/
int pos_geometric_mean_one_cycle(const int *array, size_t array_size, double *res)
{
    unsigned int pos_n = 0;
    int pos_comp = 1;
    for (size_t i = 0; i < array_size; i++)
    {
        if (array[i] > 0)
        {
            pos_comp *= array[i];
            pos_n++;
        }
    }
    if (pos_n == 0)
    {
        return NO_POS_NUMS_ERROR;
    }
    *res = n_root(pos_comp, pos_n);
    return OK;
}

int main(void)
{
    int rc;
    int array[MAX_SIZE];
    size_t size;
    double res;
    rc = input(array, &size);
    
    if (rc == IO_ERROR)
    {
        printf("PID %d: Error: Wrong input.", getpid());
        return rc;
    }
    
    if (rc == SIZE_ERROR)
    {
        printf("PID %d: Error: Array size cannot be that high.", getpid());
        return rc;
    }
    
    if (rc == EMPTY_ARRAY_ERROR)
    {
        printf("PID %d: Error: Empty array.", getpid());
        return rc;
    }

    rc = pos_geometric_mean_one_cycle(array, size, &res);

    if (rc == NO_POS_NUMS_ERROR)
    {
        printf("PID %d: Error: No positive numbers.", getpid());
        return NO_POS_NUMS_ERROR;
    }

    printf("PID %d: Geometric mean: %lf\n", getpid(), res);
    return OK;
}

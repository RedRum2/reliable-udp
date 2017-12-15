#include <stdio.h>
#include <stdlib.h>

#include "bit_array.h"


void print_array(struct bit_array *array)
{
    unsigned int i;

    for (i = 0; i < 320; i++) {
        if (check_bit(array, i))
            putchar(1);
        else
            putchar(0);
    }
    puts("\n");
}


int main()
{
    struct bit_array array;

    unsigned int i;

    reset(&array);
    print_array(&array);


    for (i = 0; i < 10; i++)
        set_bit(&array, i);
    print_array(&array);


    set_bit(&array, 34);
    set_bit(&array, 66);
    set_bit(&array, 67);
    print_array(&array);


    reset(&array);
    print_array(&array);


    return EXIT_SUCCESS;
}

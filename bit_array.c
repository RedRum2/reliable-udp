#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "bit_array.h"


/*
 * Function:	set_bit	
 * ---------------------------
 * Set the x-th bit to 1.
 * x / K is the index of the variable
 * x % K is the relative position into the variable
 * 
 * Parameters:
 * 		array:	the bit_array struct address	
 * 		x:		position of the bit to set 
 *
 * Returns: 
 * 		-1  if the position value is too big, 
 * 		 0  otherwise	
 */
int set_bit(struct bit_array *array, unsigned int x)
{
    if (x >= N_VAR * K_BIT) {
        errno = EINVAL;
        return -1;
    }

    array->bits[x / K_BIT] |= (1 << (x % K_BIT));

    return 0;
}



/*
 * Function:	check_bit	
 * ---------------------------
 * Check if the x-th bit is set to 1.
 * x / K is the index of the variable
 * x % K is the relative position into the variable
 * 
 * Parameters:
 * 		array:	the bit_array struct address	
 * 		x:		position of the bit to check
 *
 * Returns: 
 * 		-1  if the position value is too big, 
 * 		 1  if the x-th is set
 * 		 0  otherwise	
 */
int check_bit(struct bit_array *array, unsigned int x)
{
    if (x >= N_VAR * K_BIT) {
        errno = EINVAL;
        return -1;
    }

    if (array->bits[x / K_BIT] & (1 << (x % K_BIT)))
        return 1;
    else
        return 0;
}



/*
 * Function:	shift	
 * ---------------------------
 * Shift by the passed value to the right of the bits
 *
 * Parameters:
 * 		array:	the bit_array struct address	
 * 		shift:	shift value
 *
 * Returns: 
 * 		-1  if the shift value is too big, 
 * 		 0  otherwise	
 */
int shift(struct bit_array *array, unsigned int shift)
{
    uint32_t *a;
    unsigned int i, n, x;


    if (shift >= N_VAR * K_BIT) {
        errno = EINVAL;
        return -1;
    }

    a = array->bits;			// var array
    n = shift / K_BIT;			// var index
    x = shift % K_BIT;          // relative shift

    if (n) {
        for (i = 0; i + n < N_VAR; i++)
            a[i] = a[n + i];
        memset(a + i, 0, (N_VAR - i) * sizeof(uint32_t));
    }
	if (x) {
		for (i = 0; i < N_VAR - 1; i++)
			a[i] = (a[i] >> x) | (a[i + 1] << (K_BIT - x));
		a[N_VAR - 1] >>= x;
	}

    return 0;
}


/*
 * Function:	reset	
 * ---------------------------
 * Set the bit array to 0;  
 *
 * Parameters:
 * 		array:	the bit_array struct address	
 *
 * Returns: 
 * 		a pointer to the set bits
 */
void *reset(struct bit_array *array)
{
    return memset(array->bits, 0, N_VAR * sizeof(uint32_t));
}

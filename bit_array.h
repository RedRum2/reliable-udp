#ifndef _BIT_ARRAY_H
#define _BIT_ARRAY_H

#include <stdint.h>

#define N_VAR	4							// Number of variables
#define K_BIT	(8 * sizeof(uint32_t))		// Number of bits per variable

struct bit_array {
	uint32_t bits[N_VAR]; 	// N*32 total bits
};

int set_bit(struct bit_array *array, unsigned int x);
int check_bit(struct bit_array *array, unsigned int x);
int shift(struct bit_array *array, unsigned int shift);
void *reset(struct bit_array *array);

#endif /* _BIT_ARRAY_H */

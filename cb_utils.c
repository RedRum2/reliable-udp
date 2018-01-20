#include "cb_utils.h"

#include <string.h>



/*
 * Function:	cbuf_free	
 * ------------------------------------------
 * States if there is free slots into the circular buffer.
 * (A circular buffer is full if S = E + 1).
 *
 * Parameters:
 * 		start	index of the first full slot
 * 		end		index of the first empty slot
 * 		size	size of the circular buffer
 *
 * Returns:
 * 		true	if there is at least a free slot
 * 		false	otherwise
 */
bool cbuf_free(unsigned int start, unsigned int end, size_t size)
{
    unsigned int limit;

    // avoid start = end 
    limit = (end + 1) % size;
    return limit != start;
}




/*
 * Function:	memcpy_tocb	
 * ---------------------------
 * Copy data from a buffer to a circular buffer, calculates how 
 * many bytes can be copied before the end of the circular buffer and eventually 
 * split the copy. 
 *
 * Parameters:
 * 		dest_cb 	destination circular buffer 
 * 		source  	source buffer
 * 		n			the number of bytes to be copied 
 * 		begin	 	the beginning index of free memory
 * 		size	 	the size of the circular buffer
 *
 */
void memcpy_tocb(void *dest_cb, const void *source, size_t n,
                 unsigned int begin, size_t size)
{
    size_t left = size - begin;

    if (n <= left)
        // copy at once
        memcpy(dest_cb + begin, source, n);
    else {
        // copy twice
        memcpy(dest_cb + begin, source, left);
        memcpy(dest_cb, source + left, n - left);
    }
}



/*
 * Function:	memcpy_fromcb
 * ---------------------------
 * Copy data from a circular buffer to a buffer, calculates how 
 * many bytes can be copied before the end of the circular buffer and eventually 
 * split the copy. 
 *
 * Parameters:
 * 		dest_cb 	destination buffer 
 * 		source  	source circular buffer
 * 		n 			the number of bytes to be copied 
 * 		begin	 	the beginning index of data
 * 		size	 	the size of the circular buffer
 *
 */
void memcpy_fromcb(void *dest, const void *source_cb, size_t n,
                   unsigned int begin, size_t size)
{
    size_t left = size - begin;

    if (n <= left)
        // copy at once
        memcpy(dest, source_cb + begin, n);
    else {
        // copy twice
        memcpy(dest, source_cb + begin, left);
        memcpy(dest + left, source_cb, n - left);
    }
}



/*
 * Function:	data_available	
 * -----------------------------------------------------------------------
 * Calculates how many bytes of data are available in the circular buffer.
 *
 * Parameters:
 * 		s		index of the buffer at wich data start
 * 		e		index of the first empty element of the buffer
 * 		size	size in byte of the buffer
 *
 * Returns:
 * 		the number of significant bytes.
 */
size_t data_available(unsigned int s, unsigned int e, size_t size)
{
    size_t available;

    if (s == e)
        available = 0;

    else {
        if (s < e)
            available = e - s;

        else
            available = size - s + e;
    }

    return available;
}



/*
 * Function:	space_available	
 * -------------------------------------------------------------
 * Calculates how many bytes are free in the circular buffer.
 *
 * Parameters:
 * 		s		index of the buffer at wich data start
 * 		e		index of the first empty element of the buffer
 * 		size	size in byte of the buffer
 *
 * Returns:
 * 		the numeber of free bytes.
 */
size_t space_available(unsigned int s, unsigned int e, size_t size)
{
    return size - data_available(s, e, size);
}

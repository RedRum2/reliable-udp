#include "window.h"



/*
 * Function:	distance
 * -------------------------------------------------------
 * calculate the distance from the base of the window,
 * also consedering when the end of the window restart 
 * from the beginning of the circular buffer and the base
 * is still at the end.
 * The function not ensure if the seqnum index is out of
 * the window.
 *
 * Parameters:
 * 		seqnum		the index of the packet seqnum
 * 		w			the address of the window
 *
 * Returns:
 * 		the distance from the base of the window
 */
unsigned int distance(struct window *w, unsigned int seqnum)
{
    unsigned int base = w->base;

    if (seqnum < base)
        return MAXSEQNUM + seqnum - base;

    return seqnum - base;
}




/* Function: 	in_prewindow
 * -----------------------------------------------------
 * States if the position index is inside the interval [base - width; base),
 * also consedering when the base restart from the beginning 
 * of the circular buffer and the base - width index is still at the end.
 *
 * Parameters:
 *		w		the address of the window
 * 		pos		the position index
 *	Returns:
 *		true	the position index is inside the interval 
 *		false	otherwise
 */
bool in_prewindow(struct window * w, unsigned int pos)
{
    unsigned int s, base, width;

    base = w->base;
    width = w->width;
    s = base >= width ? base - width : MAXSEQNUM + base - width;

    if (s < base)
        return pos >= s && pos < base;  // p in [b - w; b)
    else
        return !(pos < s && pos >= base);   // p in [b - w; MAX] || p in [0 ; b)
}




/* Function: 	in_window
 * -----------------------------------------------------
 * States if the position index is inside the window [base ; base + width),
 * also consedering when the end of the window restart from the beginning 
 * of the circular buffer and the base is still at the end.
 *
 * Parameters:
 *		w		the address of the window
 * 		pos		the position index
 *
 *	Returns:
 *		true	the position index is inside the window
 *		false	otherwise
 */
bool in_window(struct window * w, unsigned int pos)
{
    unsigned int end, base, width;
    base = w->base;
    width = w->width;
    end = (base + width) % MAXSEQNUM;

    if (base < end)
        return pos >= base && pos < end;    // p in [b ; b + w)
    else
        return !(pos < base && pos >= end); // p in [b ; MAX] || p in [0 ; b + w)
}




/*
 * Function:	pkt_acked	
 * --------------------------------------------------------
 * check if the segment specified by seqnum is acked.
 *
 * Parameters:
 * 		w		the window to check if the seqnum is valid
 * 		seqnum	segment's sequence number
 * 		
 * 	Returns:
 * 		true	the packet is acked
 * 		false	otherwise
 */
bool pkt_acked(struct window * w, unsigned int seqnum)
{
    unsigned int i;
    int acked;

    if (in_window(w, seqnum)) {
        /* calculate relative distance from the base of the window */
        i = distance(w, seqnum);
        acked = check_bit(&w->ack_bar, i);
        if (acked == -1)
            handle_error("check_bit()");
    } else
        /* base slid over the seqnum: pkt acked */
        acked = 1;

    return acked;
}




/*
 * Function:	is_duplicate
 * -----------------------------------------------------
 * Check if the segment with relative seqnum rel_pos is a duplicate,
 * e.g. it is already arrived.
 *
 * Parameters:
 * 		w:			the window of the in-flight packets
 * 		rel_pos:	distance of the seqnum from the base of the window
 *
 * Returns:
 * 		true:	the segment is a duplicate
 * 		false:	otherwise
 */
bool is_duplicate(struct window * w, unsigned int rel_pos)
{
    int duplicate = check_bit(&w->ack_bar, rel_pos);
    if (duplicate == -1)
        handle_error("check_bit()");
    return duplicate;
}




/*
 * Function:	calc_shift
 * ------------------------
 * Calculate the number of acked packets besides the first.
 * This number is the amount of digits to shift when the base packet arrives.
 *
 * Parameters:
 * 		w	the address of the window
 *
 * Returns:
 * 		the number of digits to shift
 */
unsigned int calc_shift(struct window *w)
{
    unsigned int i;
    int retval;

    for (i = 1; i < w->width; i++) {
        retval = check_bit(&w->ack_bar, i);
        if (retval == -1)
            handle_error("check_bit");
        if (!retval)
            break;
    }
    //fprintf(stderr, "SHIFT = %u\n", i);
    return i;
}



/*
 * Function:
 * ------------------------------------------------
 * Shift window's sequence numbers of the quantity
 * specified by s.
 *
 * Parameters:
 * 		w	the address of the window
 * 		s	the amount of sequence 
 */
void shift_window(struct window *w, unsigned int s)
{
    shift(&w->ack_bar, s);
}



/*
 * Function:	update_window
 * --------------------------------------------------
 * Set the segment as acked.
 * If it match the base of the window make it slide
 * to the next unacked segment.
 *
 * Parameters:
 * 		w		the address of the window
 * 		acknum	the segment's sequence number
 */
void update_window(struct window *w, uint8_t acknum)
{
    unsigned int i, s;

    if (acknum == w->base) {

        /* shift ack bar to the first unmarked bit */
        s = calc_shift(w);
        if (shift(&w->ack_bar, s) == -1)
            handle_error("shift()");

        /* slide window */
        w->base = (w->base + s) % MAXSEQNUM;
    }

    else if (in_window(w, acknum)) {

        /* calculate distance from window's base */
        i = distance(w, acknum);
        /* mark packet as acked */
        if (set_bit(&w->ack_bar, i) == -1)
            handle_error("set_bit()");
    }
}




void fprint_window(FILE * stream, struct window *w)
{
    unsigned int i;
    int retval;

    for (i = 0; i < w->width; i++) {
        retval = check_bit(&w->ack_bar, i);
        if (retval == -1)
            handle_error("check_bit");
        if (retval)
            fputc('1', stream);
        else
            fputc('0', stream);
    }
    fprintf(stream, "\nbase:%u\n", w->base);
}

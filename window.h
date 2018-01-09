#ifndef _WINDOW_H 
#define _WINDOW_H

#include "transport.h"
#include "bit_array.h"

struct window {
	unsigned int base;
	unsigned int width;
	struct bit_array ack_bar;	// 128 bit array
};

bool in_prewindow(struct window *w, unsigned int pos);
bool in_window(struct window *w, unsigned int pos);
bool pkt_acked(struct window * w, unsigned int seqnum);
bool is_duplicate(struct window *w, unsigned int rel_pos);
unsigned int distance(struct window *w, unsigned int seqnum);
unsigned int calc_shift(struct window *w);
void shift_window(struct window *w, unsigned int s);
void update_window(struct window *w, uint8_t acknum);
void fprint_window(FILE * stream, struct window *w);

#endif /* _WINDOW_H */

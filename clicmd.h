#ifndef _CLICMD_H
#define _CLICMD_H

unsigned short get_cmdcode(const char *input);
void cli_list(void);
void cli_get(const char *filename);
void cli_put(const char *filename);

#endif /* _CLICMD_H */

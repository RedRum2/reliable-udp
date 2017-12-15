#include "basic.h"

#include <string.h>
#include <errno.h>


/*
 * Function:	extract_cmd	
 * ----------------------------------------------
 * Read the first word of the string, skipping eventually blank
 * spaces before the word.
 * This function is independent from calculator char size.
 *
 * Parameters:
 * 		line:	the address of the string containg the user input
 *
 * Returns:
 * 		the address of the command on success.
 * 		NULL if the string has length 0 or a memory allocation error
 * 			occurs.
 */
char *extract_cmd(const char *line)
{
    char *cmd;
    unsigned int begin, end;
    size_t line_len, cmd_len;

    line_len = strlen(line);

    /* invalid argument */
    if (line_len == 0) {
        errno = EINVAL;
        return NULL;
    }

    /* calculate begin index */
    begin = 0;
    while (line[begin] == ' ')
        begin++;

    /* calculate end index */
    end = begin + 1;
    while (line[end] != ' ')
        end++;

    cmd_len = end - begin;

    /* allocate memory for the cmd to return */
    if ((cmd = malloc(cmd_len + 1)) == NULL)
        return NULL;

    /* extract the cmd */
    strncpy(cmd, line + begin, cmd_len);
    cmd[cmd_len] = 0;           // terminating null byte

    return cmd;
}



/*
 *	Function:	getwordn
 *	----------------------------------------------------------------
 *	Get the n-th word of a string containing spaces separeted words.
 *	This function make the assumption that the size of char is of 1 byte.
 *
 *	Parameters:
 *		line:	the address of the string
 *		n:		the number of the word to extract
 *
 *	Returns:
 *		the address of the n-th word of the string.
 *		NULL if the string contains less than n words or a memory 
 *		allocation error occurs.
 */
char *getwordn(const char *line, unsigned int n)
{
    char *word;
    size_t word_len, line_len;

    unsigned int i;
    unsigned int begin;         // begin index of the word
    unsigned int end;           // index after end of the word 

    line_len = strlen(line);
    begin = end = 0;

    for (i = 0; i <= n; i++) {

        if (end >= line_len)
            return NULL;

        begin = end + strspn(line + end, " ");  // skip blank spaces
        end = begin + strcspn(line + begin, " ");
    }

    word_len = end - begin - 1;

    word = malloc(word_len + 1);
    if (!word)
        return NULL;

    strncpy(word, line + begin, word_len);  // extract the word
    word[word_len] = 0;         // terminating null byte

    return word;
}


/*
 * Function:	extract_filename
 * -----------------------------------------
 * Extract the file name from the input string, skipping
 * eventually blank spaces and the command.
 *
 * Parameters:
 * 		line:	the address of the input string
 * 	
 * Returns:
 * 		the address of the file name string;
 * 		NULL if no file name was found or if a memory 
 * 			allocation error occurs.
 */
char *extract_filename(const char *line)
{
    unsigned int i;
    char *filename;
    size_t line_len, name_len;

    line_len = strlen(line);

    /* skip possible blank spaces */
    i = 0;
    while (line[i] == ' ')
        i++;
    if (i == line_len) {
        errno = EINVAL;
        return NULL;
    }

    /* skip command */
    while (line[i] != ' ')
        i++;
    if (i == line_len) {
        errno = EINVAL;
        return NULL;
    }

    /* skip separation blank spaces */
    while (line[i] == ' ')
        i++;
    if (i == line_len) {
        errno = EINVAL;
        return NULL;
    }

    name_len = line_len - i;

    /* allocate memory */
    if ((filename = malloc(name_len + 1)) == NULL)
        return NULL;

    /* extract filename */
    strncpy(filename, line + i, name_len);
    filename[name_len] = 0;

    return filename;
}



ssize_t writen(int fd, const void *buf, size_t count)
{
    ssize_t w;
    size_t left = count;

    while (left > 0) {
        w = write(fd, buf, left);
        if (w == -1) {
            if (errno == EINTR) /* signal interruption */
                w = 0;
            else
                return -1;      /* error */
        }
        left -= w;
        buf += w;
    }
    return count - left;
}



ssize_t readn(int fd, void *buf, size_t count)
{
    ssize_t r;
    size_t left = count;

    while (left > 0) {
        r = read(fd, buf, left);
        if (r == -1) {
            if (errno == EINTR) // signal interruption
                continue;
            else
                return -1;      // error
        }
        if (r == 0) {           //EOF 
            continue;
            fputs("EOF read\n", stderr);
        }
        left -= r;
        buf += r;
    }
    return count - left;
}



/*
 * Function:	read_string
 * ------------------------
 * reads characters from a file until finds terminating null byte.
 *
 * Parameters:
 * 		fd:		the file to be read
 * 		buf:	the char buffer to store the string
 * 		maxlen:	the maximum number of characters to be read
 *
 * Returns:
 * 		the number of read characters
 * 		0 if nothing was read
 * 		-1 on error
 */
ssize_t read_string(int fd, void *buf, size_t maxlen)
{
    unsigned int n;
    int rc;
    char c, *p;

    p = buf;
    for (n = 1; n < maxlen; n++) {
        fputs("read..\n", stderr);
        if ((rc = read(fd, &c, 1)) == 1) {
            *p++ = c;
            fputc(c, stderr);
            if (c == '\0')
                break;
        } else if (rc == 0) {   /* read ha letto l'EOF */
            if (n == 1)
                return 0;       /* esce senza aver letto nulla */
            else
                break;
        } else
            return -1;          /* errore */
    }

    *p = '\0';                  /* per indicare la fine dell'input */
    return n;                   /* restituisce il numero di byte letti */
}



/*
 * Function:	circular_read
 * ---------------------------
 * read data from a file and put it into a circular buffer, calculates how 
 * many bytes can be written before the end of the buffer and eventually 
 * split the reading. 
 *
 * Parameters:
 * 		fd: 	 the file to be read
 * 		buffer:  the data destination buffer
 * 		begin:	 the beginning index of free memory
 * 		size:	 the size of the buffer
 * 		towrite: the number of bytes to be read
 *
 * 	Returns:
 * 		the number of read bytes, -1 in case of error.
 */
int circular_read(int fd, void *buffer, unsigned int begin, size_t size,
                  size_t toread)
{
    size_t left = size - begin;
    int r1;
    int r2;

    if (toread <= left)
        // Read once
        return readn(fd, buffer + begin, toread);
    else {
        // Read twice
        if ((r1 = readn(fd, buffer + begin, left)) == -1)
            return -1;
        if ((r2 = readn(fd, buffer, toread - left)) == -1)
            return -1;
        return r1 + r2;
    }
}



/*
 * Function:	circular_write
 * ---------------------------
 * write data from a circular buffer, calculates how many bytes can be read 
 * before the end of the buffer and eventually split the writing restarting
 * from the beginning of the buffer.
 *
 * Parameters:
 * 		fd: 	 the file to be written
 * 		buffer:  the buffer containing the data to be written
 * 		begin:	 the beginning index of significant data
 * 		size:	 the size of the buffer
 * 		towrite: the number of bytes to be written
 *
 * 	Returns:
 * 		the number of written bytes, -1 in case of error.
 */
int circular_write(int fd, const void *buffer, unsigned int begin,
                   size_t size, size_t towrite)
{
    size_t left = size - begin;
    int w1;
    int w2;

    if (towrite <= left)
        // Write once
        return writen(fd, buffer + begin, towrite);
    else {
        // Write twice
        if ((w1 = writen(fd, buffer + begin, left)) == -1)
            return -1;
        if ((w2 = writen(fd, buffer, towrite - left)) == -1)
            return -1;
        return w1 + w2;
    }
}

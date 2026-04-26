/* This programme is a shared library, for assignment 3 of COMP20200.
 *
 * Name: Cian Divily
 * Student No.: 24355116
 * Email: cian.divilly@ucdconnect.ie
 *
 * This program facilitates the user taking part in a 5-question quiz which is hosted by an external
 * server. Communication between cleint and server happens over sockets. */

#ifndef _SHARED_H
#define _SHARED_H

#include <unistd.h>

#define READ_FAILED -1 /* readUntilDelim failure value */
#define NUMQUESTIONS 5 /* Number of questions to ask */
#define SUCCESS 0      /* Function return value on success */
#define FAILURE -1     /* Function return value on failure*/

/* The function safeRead() is a wrapper for the read() systemcall. The function ensures that the
 * program will read the requested number of characters even if an interrupt occurs.
 *
 * Note: This function was created in part by modifying code available in the iserver.c source
 * provided to us
 *
 * Arguments:   fd:         The file descriptor to read from
 *              bufr:       A buffer to write the characters read to. Must have at least enough
 *                          memory allocated to it to hold 'num2Read' characters.
 *              num2Read:   Number of characters to attempt to read.
 *
 * Return Value:            Returns the number of bytes read on success, returns -1 upon failue and
 *                          sets errno accordingly (the read() systemcall handles setting errno).
 *                          If the number of bytes read is less than the requested number of bytes,
 *                          this indicates that the socket was closed on the other end. */
ssize_t safeRead(int fd, char bufr[], int num2Read);

/* The function safeWrite() is a wrapper for the write() systemcall. The function ensures that the
 * program will write the requested number of characters if an interrupt occurs.
 *
 * Note: This function was created in part by modifying code available in the iserver.c source
 * provided to us
 *
 * Arguments:   fd:         The file descriptor to write to
 *              bufr:       A buffer with the characters to write. Must have at least enough
 *                          memory allocated to it to hold 'num2Write' characters.
 *              num2Write:  Number of characters to attempt to write.
 *
 * Return Value:            Returns the number of bytes written on success, returns -1 upon failue
 *                          and sets errno accordingly (the write() systemcall handles setting
 *                          errno). If the number of bytes written is less than the requested number
 *                          of bytes, this indicates that the socket was closed on the other end. */
ssize_t safeWrite(int fd, const char bufw[], int num2Write);

/* The function readUntilDelim() reads data using the file descriptor 'fd' until either the
 * delimiter value is received or the buffer runs out of space.
 *
 * Arguments:       fd:         file descriptor to read data from.
 *                  buffer:     buffer to write values read to.
 *                  bufsize:    The maximum number of values which can be written to the buffer.
 *                  delimiter:  The value which marks the end of the data to read.
 *
 * Return Value:    returns number of characters read when the delimiter character is encountered.
 *                  Returns READ_FAILED (-1) on failure cases. */
int readUntilDelim(int fd, char buffer[], long bufsize, char delimiter);

#endif
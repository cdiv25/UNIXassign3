/* This programme is a shared library, for assignment 3 of COMP20200.
 *
 * Name: Cian Divily
 * Student No.: 24355116
 * Email: cian.divilly@ucdconnect.ie
 *
 * This program facilitates the user taking part in a 5-question quiz which is hosted by an external
 * server. Communication between cleint and server happens over sockets. */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "shared.h"

/* The function safeRead() is a wrapper for the read() systemcall. The function ensures that the
 * program will read the requested number of characters if an interrupt occurs.
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
ssize_t safeRead(int fd, char bufr[], int num2Read)
{
    ssize_t totRead = 0; /* total characters read */

    for (totRead = 0; totRead < num2Read;)
    {
        ssize_t numRead = read(fd, bufr, num2Read - totRead);
        if (numRead == 0) /* Connection closed from other end. Stop reading from file */
            break;
        if (numRead == -1) /* Read error */
        {
            if (errno == EINTR) /* Continue reading if the read error was cused by an interrupt */
                continue;
            else
            {
                fprintf(stderr, "safeReceive: Read error.\n");
                break; /* Will return -1, indicating read error */
            }
        }
        totRead += numRead;
        bufr += numRead;
    }

    return totRead; /* -1 on read error, 0 if connection closed and >0 on success */
}

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
ssize_t safeWrite(int fd, const char bufw[], int num2Write)
{
    size_t totWritten;
    for (totWritten = 0; totWritten < num2Write;)
    {
        ssize_t numWritten = write(fd, bufw, num2Write - totWritten);
        if (numWritten <= 0)
        {
            if (numWritten == -1 && errno == EINTR)
                continue;
            else
            {
                fprintf(stderr, "Write error. Errno %d.\n", errno);
                break;
            }
        }
        totWritten += numWritten;
        bufw += numWritten;
    }

    return totWritten;
}

/* The function readUntilDelim() reads data using the file descriptor 'fd' until either the
 * delimiter value is received or the buffer runs out of space.
 *
 * Arguments:       fd:         file descriptor to read data from.
 *                  buffer:     buffer to write values read to.
 *                  bufsize:    The maximum number of values which can be written to the buffer.
 *                  delimiter:  The value which marks the end of the data to read.
 *
 * Return Value:    returns number of characters read on success. If the last character written to
 *                  the buffer is not the delimiter, the maximum buffer length was reached.
 *                  Returns READ_FAILED (-1) on failure cases. */
int readUntilDelim(int fd, char buffer[], long bufsize, char delimiter)
{
    // Declare variables
    long totRead = 0; /* for loop. Increments after every byte read. */
    int rxRetVal;     /* return value of safeReceive(). */

    do // receive data one byte at a time until either delimiter value reached or buffer size limit
       // reached
    {
        if (totRead >= bufsize) // Catch buffer overlows
        {
            fprintf(stderr, "readUntilDelim(): overflowed buffer size. Buffer size is %ld\n",
                    bufsize);
            return totRead;
        }

        // Try to receive the next byte
        rxRetVal = read(fd, buffer + totRead, 1);

        // Check that the byte was received properly
        if (rxRetVal < 0) // error while reading from socket
        {
            fprintf(stderr, "readUntilDelim(): Error at index %ld reading filename\n", totRead);
            return READ_FAILED;
        }
        else if (rxRetVal == 0) // socket was closed
        {
            fprintf(stderr,
                    "readUntilDelim(): socket was closed before delimiter was encountered. total "
                    "read: %ld\n",
                    totRead);
            return 0;
        }

        totRead++; // advance to next byte

    } while (buffer[totRead - 1] != delimiter); // stop when the delimiter is the last byte received

    return totRead;
}
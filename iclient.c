/* This programme is the client-side part of assignment 3 of COMP20200.
 *
 * Name: Cian Divily
 * Student No.: 24355116
 * Email: cian.divilly@ucdconnect.ie
 *
 * This program facilitates the user taking part in a 5-question quiz which is hosted by an external
 * server. Communication between cleint and server happens over sockets.
 * Note: The logic for creating, connecting and closing sockets was based on the TCP socket sources
 * on moodle. We were told that we should use these as a template for programmes which we write */

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "shared.h" /* Functions that handle communication over socket */

/******************* Macros ****************/
#define BUFLEN 300 /* Big enough for welcome message */

/******** Function Declarations **********/
void playRound(int cfd);
char welcomeUser(int cfd);
int doQuestions(int cfd);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <IP address of server> <port number>.\n", argv[0]);
        exit(-1);
    }

    struct sockaddr_in serverAddress;

    memset(&serverAddress, 0, sizeof(struct sockaddr_in));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddress.sin_port = htons(atoi(argv[2]));

    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1)
    {
        fprintf(stderr, "socket() error.\n");
        exit(-1);
    }

    int rc = connect(cfd, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr));
    if (rc == -1)
    {
        fprintf(stderr, "connect() error, errno %d.\n", errno);
        exit(-1);
    }

    playRound(cfd);

    if (close(cfd) == -1) /* Close connection */
    {
        fprintf(stderr, "close error.\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

/* The function playRound() facilitates the user playing a round of a 5-question quiz by
 * communicating with a server using the socket described by the cfd file descriptor.
 *
 * First, a welcome message is received asking the user whether theywant to continue the quiz or
 * quit. Next, the user's response is sent to the server. If the user wants to continue the quiz,
 * the server sends 5 questions, 1-by-1. For each question, the client sends an answer. For each
 * answer, the server sends a message saying whether the answer is right or wrong. At the end, the
 * server sends the score that the user got.
 *
 * Arguments:   cfd:    File descriptor describing socket used for communication with server */
void playRound(int cfd)
{
    char buffer[BUFLEN];   /* Buffer for reading/writing to socket */
    int numReceived = 0;   /* Number of bytes received */
    char shouldQuit = 'Q'; /* Indicates whether user decided to quit quiz*/

    /* Receive welcome message and decide whether to continue */
    shouldQuit = welcomeUser(cfd);
    if (shouldQuit == 'q' || shouldQuit == 'Q')
        return; /* Quit quiz */

    /* Receive questions, get answers, send answers*/
    if (doQuestions(cfd) == FAILURE)
        return; /* Error communicating over socket. Quit quiz*/

    /* Quiz over. Get score from teh server */
    numReceived = readUntilDelim(cfd, buffer, sizeof(buffer), '\0');
    if (numReceived < 0)
    {
        fprintf(stderr, "Error: Failed to receive score\n");
        return;
    }
    else if (numReceived == 0)
    {
        fprintf(stderr, "Error: Socket closed before score received\n");
        return;
    }
    printf("%s", buffer); /* Print score message */
}

/* The function welcomeUser() receives the welcome message from the server, displays this message,
 * gets the user's reponse for whether to quit and sends this response to the server.
 *
 * Arguments:   cfd:    file descriptor for communication with server over socket.
 *
 * Return value:        Returns 'q' or 'Q' if the user chooses to quit the quiz. Returns 'Q' if an
 *                      error occurs sending or receiving data over the socket. Returns 'y' or 'Y'
 * if the user decides to continue with the quiz and no error occurs */
char welcomeUser(int cfd)
{
    char buffer[BUFLEN]; /* Buffer for reading/writing to socket */
    int numReceived = 0; /* Number of bytes received */

    /* Receive and display welcome message. All messages are terminated by '\0' */
    numReceived = readUntilDelim(cfd, buffer, BUFLEN, '\0');
    if (numReceived < 0)
    {
        fprintf(stderr, "Error: Failed to receive welcome message\n");
        return 'Q';
    }
    else if (numReceived == 0)
    {
        fprintf(stderr, "Error: Socket closed on other end before welcome received\n");
        return 'Q';
    }
    printf("%s", buffer); /* Print welcome message */

    /* Get user's response on whether to continue or quit */
    buffer[0] = getchar();
    if (buffer[0] != '\n')
        while (getchar() != '\n'); /* Consume any tailing characters */
    buffer[1] = '\0';              /* Null-terminate for sending to server */

    /* Handle invalid inputs gracefully */
    if (buffer[0] != 'y' && buffer[0] != 'Y' && buffer[0] != 'q' && buffer[0] != 'Q')
    {
        printf("Invalid input! Quitting quiz...\n");
        buffer[0] = 'q';
    }

    /* Send continue/quit (including terminating '\0') */
    int numSent = 0; /* Number of bytes sent */
    numSent = safeWrite(cfd, buffer, strlen(buffer) + 1);
    if (numSent < 0)
    {
        fprintf(stderr, "Error: Could not send continue/quit response\n");
        return 'Q';
    }
    else if (numSent == 0)
    {
        fprintf(stderr, "Error: Socket closed. Could not send continue/quit response\n");
        return 'Q';
    }

    return buffer[0];
}

/* The function doQuestions() receives questions from the server, one-by-one.
 * For each question, the function reads in the user's answer via stdin, sends the answer to the
 * server and receives a message from the server, indicating wthether the answer was correct.
 *
 * Arguments:   cfd:    file descriptor for communication with server over socket
 *
 * Return value:    Returns FAILURE if there is an issue communicatiing over the socket. Returns
 *                  SUCCESS otherwise */
int doQuestions(int cfd)
{
    char buffer[BUFLEN]; /* Buffer for reading/writing to socket */
    int numReceived = 0; /* Number of bytes received */

    /* Handle each question */
    for (int questionNum = 0; questionNum < NUMQUESTIONS; questionNum++)
    {
        /* Receive question */
        numReceived = readUntilDelim(cfd, buffer, sizeof(buffer), '\0');
        if (numReceived < 0)
        {
            fprintf(stderr, "Error: Failed to receive question\n");
            return FAILURE;
        }
        else if (numReceived == 0)
        {
            fprintf(stderr, "Error: Socket closed before question received\n");
            return FAILURE;
        }
        printf("%s\n", buffer); /* Print question */

        /* Get user's answer */
        fgets(buffer, sizeof(buffer), stdin);
        if (buffer[strlen(buffer) - 1] != '\n')
            while (getchar() != '\n'); /* Input overflowed buffer. Clear trailing characters */
        else
            buffer[strlen(buffer) - 1] = '\0'; /* Remove newline (for strcmp with correct answer)*/

        /* Send Answer (including terminating '\0')*/
        int numSent = safeWrite(cfd, buffer, strlen(buffer) + 1);
        if (numSent < 0)
        {
            fprintf(stderr, "Error: Could not send answer\n");
            return FAILURE;
        }
        else if (numSent == 0)
        {
            fprintf(stderr, "Error: Socket closed before answer could be sent\n");
            return FAILURE;
        }

        /* Get Right/Wrong answer message from server */
        numReceived = readUntilDelim(cfd, buffer, sizeof(buffer), '\0');
        if (numReceived < 0)
        {
            fprintf(stderr, "Error: Failed to receive right/wrong answer message\n");
            return FAILURE;
        }
        else if (numReceived == 0)
        {
            fprintf(stderr, "Error: Failed to receive right/wrong answer message\n");
            return FAILURE;
        }
        printf("%s", buffer); /* Display the right/wrong answer message */
    }

    return SUCCESS;
}
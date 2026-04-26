/* This programme is the server-side part of assignment 3 of COMP20200.
 *
 * Name: Cian Divily
 * Student No.: 24355116
 * Email: cian.divilly@ucdconnect.ie
 *
 * This program facilitates the user taking part in a 5-question quiz which is hosted by an external
 * server. Communication between client and server happens over sockets.
 *
 * Note: The logic for creating, connecting and closing sockets was based on the TCP socket sources
 * on moodle. We were told that we should use these as a template for programmes which we write */

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "QuizDB.h" /* Quiz question sand answers */
#include "shared.h" /* Functions that handle communication over socket */

/******************* Macros ****************/
#define BACKLOG 32   /* Maximum number of clients waiting to connect */
#define IP_INDEX 1   /* Index of IPv4 address in argv */
#define PORT_INDEX 2 /* Index of port number in argv */
#define BUFLEN 300   /* Big enough to hold the welcome message*/

/******************* Function Declarations *******************/
void hostQuiz(int cfd);
int sendQuestions(int cfd);

int main(int argc, char *argv[])
{
    /* Ensure that the correct number of arguments are passed */
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <IPv4 address> <port number>.\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Set seed for random question selection */
    srand(time(NULL));

    /* Create socket for listening for clients */
    struct sockaddr_in serverAddress;

    memset(&serverAddress, 0, sizeof(struct sockaddr_in));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr =
        inet_addr(argv[IP_INDEX]); /* Convert IP Address to machine-readable format */
    serverAddress.sin_port = htons(atoi(argv[PORT_INDEX])); /* Convert port num to integer*/

    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        fprintf(stderr, "socket() error.\n");
        exit(-1);
    }

    /*
     * This socket option allows you to reuse the server endpoint
     * immediately after you have terminated it.
     */
    int optval = 1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        exit(-1);

    int rc = bind(lfd, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr));
    if (rc == -1)
    {
        fprintf(stderr, "bind() error.\n");
        exit(-1);
    }

    if (listen(lfd, BACKLOG) == -1)
        exit(-1);

    printf("<Listening on %s:%s>\n", argv[IP_INDEX], argv[PORT_INDEX]);
    printf("<Press ctrl-C to terminate>\n");

    for (;;) /* Handle clients iteratively */
    {

        struct sockaddr_storage claddr;
        socklen_t addrlen = sizeof(struct sockaddr_storage);
        int cfd = accept(lfd, (struct sockaddr *)&claddr, &addrlen);
        if (cfd == -1)
        {
            fprintf(stderr, "Failed to accept connection. errno is %d", errno);
            continue;
        }

        hostQuiz(cfd); /* Primary logic of assignment */

        if (close(cfd) == -1) /* Close connection */
        {
            fprintf(stderr, "close error.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (close(lfd) == -1) /* Close listening socket */
    {
        fprintf(stderr, "close error.\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

/* The function hostQuiz() presents a quiz to a client, using the socket described by 'cfd'.
 * The client is welcomed, the client indicates whether to continue the quiz or to quit,
 * 5 questions are sent to the client and finally, the score is sent to the client and the quiz is
 * concluded.
 *
 * For each question, the client sends their answer and the server sends a response saying whetehr
 * the answer is right or wrong.
 *
 * Arguments:       cfd:    File descriptor for writing to socket and reading from socket */
void hostQuiz(int cfd)
{
    /* Buffer used to store bytes to send and bytes received. Initialised with welcome message.
     * This is the longest message which will be in the buffer. */
    char buffer[BUFLEN] = "Welcome to Unix Programming Quiz!\n"
                          "The quiz comprises five questions posed to you one after the other.\n"
                          "You have only one attempt to answer a question.\n"
                          "Your final score will be sent to you after conclusion of the quiz.\n"
                          "To start the quiz, press Y and <enter>. \n"
                          "To quit the quiz, press q and <enter>.\n";

    /* Send welcome message, including '\0' */
    int retVal = safeWrite(cfd, buffer, strlen(buffer) + 1);
    if (retVal < 0)
    {
        fprintf(stderr, "Write error\n");
        return;
    }
    else if (retVal < strlen(buffer) + 1)
    {
        fprintf(stderr, "Socket closed early\n");
        return;
    }

    /* Read whether client wants to continue or quit. Response must be terminated by '\0' */
    int totRead = 0; /* Number of bytes read */
    do
    {
        /* Receive from client until '\0' received (marks end of response). */
        totRead = readUntilDelim(cfd, buffer, sizeof(buffer), '\0');
        if (totRead < 0)
        {
            fprintf(stderr, "Read error\n");
            return;
        }
        else if (totRead == 0)
        {
            fprintf(stderr, "Socket closed early\n");
            return;
        }
    } while (buffer[totRead - 1] != '\0'); /* Read until '\0',
                                            * overwriting buffer if needed */

    if (buffer[0] == 'q' || buffer[0] == 'Q')
        return; /* Client wants to quit round */

    /* Send questions to client, receive answers and send "right/wrong answer" message */
    int score = sendQuestions(cfd);

    /* Send score, including '\0' */
    snprintf(buffer, sizeof(buffer), "Your quiz score is %d/%d. Goodbye.\n", score, NUMQUESTIONS);
    retVal = safeWrite(cfd, buffer, strlen(buffer) + 1);
    if (retVal < 0)
    {
        fprintf(stderr, "Write error\n");
        return;
    }
    else if (retVal < strlen(buffer) + 1)
    {
        fprintf(stderr, "Socket closed early\n");
        return;
    }
}

/* The function sendQuestions() picks questions from the database. For each question, the function
 * sends tha question to the client, receives the client's answer, updates the client's score and
 * sends a message telling the client whether the answer was correct (and what the right answer was
 * if necessary)
 *
 * Arguments:   cfd:    file descriptor for communication with client over socket
 *
 * Return Value:        Returns FAILURE (-1) if some error occurs communicating over the socket (or
 * if the client closes the socket). Otherwise, returns the client's score */
int sendQuestions(int cfd)
{
    int score = 0;                    /* Number of questions client gets right */
    int questionsAsked[NUMQUESTIONS]; /* Stores questions already asked */
    int totRead;                      /* return values of functions. Used for error checking */
    char buffer[BUFLEN];              /* Buffer for sending/receiving data over socket*/

    for (int questionNum = 0; questionNum < NUMQUESTIONS; questionNum++)
    {
        int curQuestion = rand() % (sizeof(QuizQ) / sizeof(QuizQ[0])); /* Choose random question */

        /* Ensure that question was not already asked */
        for (int i = 0; i < questionNum; i++)
        {
            if (questionsAsked[i] == curQuestion) /* Question matches a previous question */
            {
                /* Choose another question */
                curQuestion = rand() % (sizeof(QuizQ) / sizeof(QuizQ[0]));
                i = 0; /* Check against previous questions again, from the start */
            }
        }

        /* Add question to list of previously asked questions */
        questionsAsked[questionNum] = curQuestion;

        /* Send question, including '\0' */
        totRead = safeWrite(cfd, QuizQ[curQuestion], strlen(QuizQ[curQuestion]) + 1);
        if (totRead < 0)
        {
            fprintf(stderr, "Write error\n");
            return FAILURE;
        }
        else if (totRead < strlen(QuizQ[curQuestion]) + 1)
        {
            fprintf(stderr, "Socket closed early\n");
            return FAILURE;
        }

        /* Read client's response. Response must be terminated by '\0' */
        int totRead = 0;          /* Number of bytes read */
        int bufferOverflowed = 0; /* Flag, marks that the response was too big for the buffer */
        do
        {
            /* Receive from client until '\0' received (marks end of response). */
            totRead = readUntilDelim(cfd, buffer, sizeof(buffer), '\0');
            if (totRead < 0)
            {
                fprintf(stderr, "Read error\n");
                return FAILURE;
            }
            else if (totRead == 0)
            {
                fprintf(stderr, "Socket closed early\n");
                return FAILURE;
            }

            /* If the buffer was not big enough to hold answer, we can assume it is wrong since
             * the buffer is longer than the longest correct answer. Just record that it overflowed
             * and continue receiving until '\0', overwriting buffer each iteration */
            if (buffer[totRead - 1] != '\0')
            {
                fprintf(stderr, "Buffer overflowed\n");
                bufferOverflowed = 1;
            }
        } while (buffer[totRead - 1] != '\0'); /* Read until '\0',
                                                * overwriting buffer if needed */

        /* Check if answer was right and inform user */
        if (bufferOverflowed || strcmp(buffer, QuizA[curQuestion]) != 0) /* Answer is wrong */

            snprintf(buffer, sizeof(buffer), "Wrong Answer. Right answer is %s.\n",
                     QuizA[curQuestion]);
        else
        {
            score++; /* Increment number of correct answers */
            strcpy(buffer, "Right Answer.\n");
        }

        /* Send answer, including '\0' */
        totRead = safeWrite(cfd, buffer, strlen(buffer) + 1);
        if (totRead < 0)
        {
            fprintf(stderr, "Write error\n");
            return FAILURE;
        }
        else if (totRead < strlen(buffer) + 1)
        {
            fprintf(stderr, "Socket closed early\n");
            return FAILURE;
        }
    }

    return score;
}
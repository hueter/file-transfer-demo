/**
 * Michael Hueter
 * CS 372 Networking Fall 2016
 * Program 2 - File Transfer (Server Component)
 * 27 November 2016
 * Sources Cited:
 *  [1] http://beej.us/guide/bgnet/
 *	[2] Advanced Programming in the UNIX Environment, 3rd Edition by W. Richard Stevens
 *	[3] http://stackoverflow.com/questions/4072190/check-if-input-is-integer-type-in-c
 *  [4] https://linux.die.net/man/2/read 
 *	[5] http://pubs.opengroup.org/onlinepubs/009695399/functions/opendir.html
 *  [6] http://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
 *  [7] http://pubs.opengroup.org/onlinepubs/009695399/functions/bzero.html
 *  [8] http://stackoverflow.com/questions/12810587/extracting-ip-address-and-port-info-from-sockaddr-storage
 *  [9] http://man7.org/linux/man-pages/man3/getnameinfo.3.html
 *  [10] http://stackoverflow.com/questions/11198604/c-split-string-into-an-array-of-strings
 *  [11] http://stackoverflow.com/questions/4217037/catch-ctrl-c-in-c
 *  [12] https://en.wikibooks.org/wiki/A_Little_C_Primer/C_File-IO_Through_System_Calls
 *  [13] http://stackoverflow.com/questions/2014033/send-and-receive-a-file-in-socket-programming-in-linux-with-c-c-gcc-g
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

// this global variable is for SIGTERM handling [11]
static volatile int done = 0;

int validatePort(char *port)
{
    /**
	 * This function makes sure the port number from the command line is a valid integer in the correct range
	 *  Return 1 if validation error, or 0 if it's OK
	 */
    int portNumber = atoi(port);

    // sorry for the verboseness, had to debug this one quite a few times
    if (portNumber < 1024)
    {
        return -1;
    }
    else if (portNumber > 65535)
    {
        return -1;
    }
    else if (portNumber == 30021)
    {
        return -1;
    }
    else if (portNumber == 30020)
    {
        return -1;
    }

    return 0;
}

char **parseCommand(char *command)
{
    /**
	 * This function takes the command string from the client and parses it into an array
	 *  Returns the string array [10]
	 */

    int spaces = 0;

    char *token = strtok(command, " ");
    char **commandBuffer = NULL;

    // split string and append tokens to 'res'
    while (token)
    {
        commandBuffer = realloc(commandBuffer, sizeof(char *) * ++spaces);

        if (commandBuffer == NULL)
        {
            exit(-1); // memory allocation failed
        }

        commandBuffer[spaces - 1] = token;
        token = strtok(NULL, " ");
    }

    // realloc one extra element for the last NULL
    commandBuffer = realloc(commandBuffer, sizeof(char *) * (spaces + 1));
    commandBuffer[spaces] = 0;

    return commandBuffer;
}

void listFilesCmd(int dataSock)
{
    /**
	* This sends a directory listing over the data socket
	*/
    int status = 0;
    char directoryListing[500];
    memset(directoryListing, '\0', 500);
    // List directories [6]
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            strcat(directoryListing, dir->d_name);
            strcat(directoryListing, "\n");
        }
    }
    status = send(dataSock, directoryListing, 500, 0);
    if (status < 0)
    {
        fprintf(stderr, "     !  Error sending directory listing.");
    }
}

int sendFileCmd(int dataSock, char *filename)
{
    /**
	* This sends a file over the data socket [12] [13]
	*/
    char fileNotFound[23] = "Error: File not found.\0";
    char readError[25] = "Error: File read error.\0";
    int fd;
    fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "\n     !  Error: Client-Requested File Not Found.\n");
        write(dataSock, fileNotFound, 23);
        return -1;
    }
    else
    {
        char buffer[4096];
        while (1)
        {
            // Read data into buffer.  We may not have enough to fill up buffer, so we
            // store how many bytes were actually read in bytes_read.
            int bytes_read = read(fd, buffer, sizeof(buffer));
            if (bytes_read == 0) // We're done reading from the file
                break;

            if (bytes_read < 0)
            {
                fprintf(stderr, "     !  Error: File Read Error.");
                write(dataSock, readError, 25);
                return -1;
            }

            // You need a loop for the write, because not all of the data may be written
            // in one call; write will return how many bytes were written. p keeps
            // track of where in the buffer we are, while we decrement bytes_read
            // to keep track of how many bytes are left to write.
            void *p = buffer;
            while (bytes_read > 0)
            {
                int bytes_written = write(dataSock, p, bytes_read);
                if (bytes_written <= 0)
                {
                    fprintf(stderr, "     !  Socket Write Error.");
                    return -1;
                }
                bytes_read -= bytes_written;
                p += bytes_written;
            }
        }
    }

    return 0;
}

int handleRequest(int sock, char *client)
{
    /**
	 * This function takes an active socket with an accepted connection, receives a command from the client,
     *   calls a function to parse the command, then calls respective functions completing the command
     *   and returns the response over a data connection.
     *  
	 */
    char clientCommand[500]; // the 500-char string sent by the client program
    bzero(clientCommand, 500);
    int numberOfArgs = 0;
    char **args;                        // hold an array of command arguments after splitting
    int dataSocket;                     // file descriptor for the data connection
    char *dataPort;                     // string form of the data port
    char *filename;                     // string to hold filename arg
    struct sockaddr_in receiverAddress; // container for the client's address
    struct hostent *receiver;           // struct for all client info
    int status;                         // generic error handling int
    int commandType = 0;                // based on # of args, what type of response to sendfile

    recv(sock, clientCommand, 500, 0);

    args = parseCommand(clientCommand);
    while (args[numberOfArgs] != NULL)
    {
        numberOfArgs++;
    }

    // two arguments implies an "-l" command
    if (numberOfArgs == 2)
    {
        dataPort = args[1];
        status = validatePort(dataPort);
        if (status < 0)
        {
            fprintf(stderr, "   ! Error: Data Port Number must be an integer between 1024 and 65535 and not 30021 or 30020, you passed: '%s'\n", args[1]);
        }
        commandType = 1;
    }
    else // otherwise we have a "-g <filename>" command
    {
        filename = args[1];
        dataPort = args[2];
        status = validatePort(dataPort);
        if (status < 0)
        {
            fprintf(stderr, "   ! Error: Data Port Number must be an integer between 1024 and 65535 and not 30021 or 30020, you passed: '%s'\n", args[2]);
        }
        commandType = 2;
    }

    // Get the Socket file descriptor
    dataSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (dataSocket < 0)
    {
        fprintf(stderr, "   ! Socket Error: Error initializing data socket.\n");
        return -1;
    }

    // setup new socket
    receiver = gethostbyname(client);
    if (receiver == NULL)
    {
        fprintf(stderr, "   ! Connection Error: Client host not reachable.\n");
        return -1;
    }
    // allocate memory for server
    bzero((char *)&receiverAddress, sizeof(receiverAddress));
    // IPv4
    receiverAddress.sin_family = AF_INET;
    // bcopy - copy byte sequence of server address
    bcopy((char *)receiver->h_addr, (char *)&receiverAddress.sin_addr.s_addr, receiver->h_length);
    // Convert multi-byte integer types from host byte order to network byte order
    receiverAddress.sin_port = htons(atoi(dataPort));

    // wait 1 second and then connect to client on new socket
    sleep(1);
    status = connect(dataSocket, (struct sockaddr *)&receiverAddress, sizeof(struct sockaddr));
    if (status < 0)
    {
        fprintf(stderr, "   ! Connection Error: Could not establish data connection with %s:%s\n", client, dataPort);
        return -1;
    }
    printf("\n  ++  Data Connection Established with %s:%s\n", client, dataPort);

    if (commandType == 1)
    {
        listFilesCmd(dataSocket);
        printf("\n   ***  Sending directory contents to %s:%s\n", client, dataPort);
    }
    else
    {
        printf("\n   ***  Sending file '%s' contents to %s:%s\n", filename, client, dataPort);
        status = sendFileCmd(dataSocket, filename);
        if (status < 0)
        {
            return -1;
        }
    }

    free(args);
    close(dataSocket);
    printf("\n  --  Data Connection Closed with %s:%s\n", client, dataPort);

    return 0;
};

int startup(char *port)
{
    /**
	 * This function takes the port number, initializes a server socket, and returns the active socket file descriptor [1]
	 */
    int sockfd; // socket file descriptor
    struct addrinfo hints, *res;
    int optVal = 1; // for socket options
    int status;     // generic error response holder

    // first, load up address structs with getaddrinfo():

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    getaddrinfo(NULL, port, &hints, &res);

    // Initialize the file descriptor of a new socket [1]
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0)
    {
        fprintf(stderr, "Socket Error: Error initializing socket.\n");
        exit(1);
    }

    // this socket option allows other sockets to bind on this port [1]
    status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));
    if (status < 0)
    {
        fprintf(stderr, "Socket Error: Error initializing socket reusability.\n");
        exit(1);
    }

    // this is where we bind our socket to the local port [1]
    status = bind(sockfd, res->ai_addr, res->ai_addrlen);
    ;
    if (status < 0)
    {
        fprintf(stderr, "Socket Error: Error binding host to port %s.\n", port);
        exit(1);
    }

    // now we're actually a server because we're listening to connections (arbitrarily set to 5) [1]
    status = listen(sockfd, 5);
    if (status < 0)
    {
        fprintf(stderr, "Socket Error: Error Setting Up Socket Listener.\n");
        exit(1);
    }

    printf("\nFile Transfer Server Listening on Port %s...\n", port);

    return sockfd;
};

void shutDown(int signum)
{
    /**
	 * This is a generic sigterm handler activated when Ctrl-C is pressed. [11]
	 */
    done = 1;
    printf("\nServer will shut down after next request...\n");
}

int main(int argc, char **argv)
{
    char *port;                         // port which we use to start server on
    int controlSocket;                  // the socket which will start listening for connections
    int activeSocket;                   // an instance of controlSocket that is actively accepting connections
    struct sockaddr_storage clientAddr; // store information about the incoming connection
    socklen_t addressSize;              // a generic variable set to the size of a sockaddr struct
    int status = 0;

    // Must have exactly two args
    if (argc != 2)
    {
        fprintf(stderr, "Usage: ftserver <portNumber>\n");
        exit(1);
    }

    // Extra validation for port argument
    port = argv[1];
    status = validatePort(port);
    if (status < 0)
    {
        fprintf(stderr, "Error: Port Number must be an integer between 1024 and 65535 and not 30021 or 30020, you passed: '%s'\n", argv[1]);
        exit(1);
    }
    else
    {
        // after validation, read in port for real
        // build a listening controlSocket with the startup function
        controlSocket = startup(port);
    }

    // SetUp SIGTERM handling so we can exit gracefully when the user enters Ctrl+C [11]
    signal(SIGINT, shutDown);

    while (done == 0)
    {
        fflush(stdout);
        // spin off another socket for an active connection
        activeSocket = accept(controlSocket, (struct sockaddr *)&clientAddr, &addressSize);
        if (activeSocket < 0)
        {
            fprintf(stderr, "Connection Error: Active Socket Failed to Accept Connection.\n");
            exit(1);
        }

        // the following is simply to retrieve hostname information [8, 9]
        char clientHost[NI_MAXHOST];
        char clientPort[NI_MAXSERV];
        int rc = getnameinfo((struct sockaddr *)&clientAddr, addressSize, clientHost, sizeof(clientHost), clientPort, sizeof(clientPort), 0);
        if (rc == 0)
        {
            printf("\n + Connection from %s\n", clientHost);
        }
        else
        {
            fprintf(stderr, "\n   ! Connection Error: There was an error reading hostname.\n");
        }

        handleRequest(activeSocket, clientHost);

        // make sure to cleanup
        close(activeSocket);
        printf("\n - Connection to %s closed\n", clientHost);
    }

    // make sure to cleanup
    close(controlSocket);
    printf("\nCleanup done. Bye!\n");
    fflush(stdout);

    exit(0);
}
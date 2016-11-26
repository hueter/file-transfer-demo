/**
 * Michael Hueter
 * CS 372 Networking Fall 2016
 * Program 2 - File Transfer (Server Component)
 * 27 November 2016
 * References: [1] http://beej.us/guide/bgnet/
 *			   [2] Advanced Programming in the UNIX Environment, 3rd Edition by W. Richard Stevens
 *			   [3] http://stackoverflow.com/questions/4072190/check-if-input-is-integer-type-in-c
 *             [4] https://linux.die.net/man/2/read 
 *			   [5] http://pubs.opengroup.org/onlinepubs/009695399/functions/opendir.html
 *             [6] http://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <dirent.h>

int validatePort(char *port)
{
    /**
	 * This function makes sure the port number from the command line is a valid integer in the correct range
	 *  Return 1 if validation error, or 0 if it's OK
	 */
    int portNo = atoi(port);
    if ((isdigit(portNo) != 0) || (portNo < 1024) || portNo > 65535 || portNo == 30021 || portNo == 30020)
    {
        return -1;
    }

    return 0;
}

void handleRequest(int *serverSocket)
{
    /**
	 * This function takes the port number, initializes a server socket, and returns the active socket file descriptor
	 */
    char clientBuffer[500];
    char *token;
    int argCount = 0;
    char **clientArgs = (char **)malloc(sizeof(char *) * 512);
    char *clientCommand;
    int dataPort;
    char *file;
    int status;

    // split client buffer on whitespace to get individual arguments
    // http://www.cplusplus.com/reference/cstring/strtok/
    // https://msdn.microsoft.com/en-us/library/2c8d19sb(v=vs.71).aspx
    token = strtok(clientBuffer, " ");
    while (token != NULL)
    {
        clientArgs[argCount] = token;
        token = strtok(NULL, " ");
        argCount++;
    }

    argCount -= 1; // go one back after the loop to get the last real element

    // the final element should be the dataPort number
    status = validatePort(clientArgs[argCount]);
    if (status < 0)
    {
        fprintf(stderr, "Error: Client passed bad data port number '%s'", clientArgs[argCount]);
        exit(1);
    }
    dataPort = atoi(clientArgs[argCount]); // set final argument to dataPort number
    clientCommand = clientArgs[0];
    // More than 2 arguments indicates a file
    if (argCount > 1)
    {
        // file should always be second arg
        file = clientArgs[1];
    }
    if (strcmp(clientCommand, "-l") == 0)
    {
    }
    else if (strcmp(clientCommand, "-g") == 0)
    {
        // get file
    }
    else
    {
        // bad command
    }
};

void startup(int port)
{
    /**
	 * This function takes the port number, initializes a server socket, and returns the active socket file descriptor
	 */
    struct sockaddr_in serverAddress;
    int sock;          // socket file descriptor
    int status;        // generic error response holder
    int optVal = 1;    // for socket options
    int controlSocket; // our main TCP socket for transactions

    // Initialize the file descriptor of a new socket
    // http://beej.us/guide/bgnet/output/html/multipage/syscalls.html#socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        fprintf(stderr, "Socket Error: Error initializing socket.\n");
        exit(1);
    }

    // ---- Initial Socket Configuration
    serverAddress.sin_family = AF_INET;                                  // we're only going to use IPv4 family here
    serverAddress.sin_port = htons(port);                                // set port, convert to Big-Endian Network Byte Order
    memset(serverAddress.sin_zero, '\0', sizeof serverAddress.sin_zero); // clear memory for the serverAddress

    // this socket option allows other sockets to bind on this port
    // http://beej.us/guide/bgnet/output/html/multipage/setsockoptman.html
    status = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int));
    if (status < 0)
    {
        fprintf(stderr, "Socket Error: Error initializing socket reuse.\n");
        exit(1);
    }

    // this is where we bind our socket to the local port
    // http://beej.us/guide/bgnet/output/html/multipage/syscalls.html#bind
    status = bind(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (status < 0)
    {
        fprintf(stderr, "Socket Error: Error binding host to port %d.\n", port);
        exit(1);
    }

    // now we're actually a server because we're listening to connections (arbitrarily set to 5)
    // http://beej.us/guide/bgnet/output/html/multipage/syscalls.html#listen
    status = listen(sock, 5);
    if (status < 0)
    {
        fprintf(stderr, "Socket Error: Error Setting Up Socket Listener.\n");
        exit(1);
    }

    printf("\nFile Transfer Server Listening on Port %d", port);

    // start up the TCP control connection
    while (1)
    {
        struct sockaddr_storage clientAddress;
        socklen_t addr_size;

        controlSocket = accept(sock, (struct sockaddr *)&clientAddress, &addr_size);
        if (controlSocket < 0)
        {
            fprintf(stderr, "Error Establishing TCP Control Socket.");
            exit(1);
        }
        printf("Connection Opened.");
    }
};

void listFilesCmd(int *controlSock, int *dataSock)
{
    /**
	* This pointers to the control and data sockets and sends a directory listing
	*/
    int status = 0;
    char directoryListing[500];
    memset(directoryListing, '\0', 500);
    // http://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
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
    status = send(*controlSock, directoryListing, 500, 0);
    if (status < 0)
    {
        fprintf(stderr, "Error sending directory listing.");
    }
}

int shutDown();

int main(int argc, char **argv)
{
    int portNumber; // port which we use to start server on

    // Must have exactly two args
    if (argc != 2)
    {
        fprintf(stderr, "Usage: ftserver <portNumber>\n");
        exit(1);
    }

    // Extra validation for port argument
    if (validatePort(argv[1]) != 0)
    {
        fprintf(stderr, "Error: Port Number must be an integer between 1024 and 65535 and not 30021 or 30020, you passed: '%s'\n", argv[1]);
        exit(1);
    }

    portNumber = atoi(argv[1]);
    startup(portNumber);

    exit(0);
}
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
	* This pointers to the control and data sockets and sends a directory listing
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
        fprintf(stderr, "Error sending directory listing.");
    }
}

int handleRequest(int sock, char *client)
{
    /**
	 * This function takes an active socket with an accepted connection, receives a command from the client,
     *   calls a function to parse the command, then calls respective functions completing the command
     *   and returns the response over a data connection.
     *  
	 */
    char clientCommand[500];
    bzero(clientCommand, 500);
    int numberOfArgs = 0;
    char **args;
    int dataSocket;
    int dataPort;
    struct sockaddr_in receiverAddress;
    struct hostent *receiver;
    int status;
    int commandType = 0;

    recv(sock, clientCommand, 500, 0);

    args = parseCommand(clientCommand);
    while (args[numberOfArgs] != NULL) {
        numberOfArgs++;
    }

    if (numberOfArgs == 2)
    {
        status = validatePort(args[1]);
        if (status < 0)
        {
            fprintf(stderr, "Error: Data Port Number must be an integer between 1024 and 65535 and not 30021 or 30020, you passed: '%s'\n", args[1]);
        }
        dataPort = atoi(args[1]);
        commandType = 1;
    }
    else
    {
        status = validatePort(args[2]);
        if (status < 0)
        {
            fprintf(stderr, "Error: Data Port Number must be an integer between 1024 and 65535 and not 30021 or 30020, you passed: '%s'\n", args[1]);
        }
        dataPort = atoi(args[2]);
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
	if (receiver == NULL) {
		fprintf(stderr, "   ! Connection Error: Client host not reachable.\n");
		return -1;
	}
    // allocate memory for server
   	bzero((char *) &receiverAddress, sizeof(receiverAddress));
	// IPv4
    receiverAddress.sin_family = AF_INET;
	// bcopy - copy byte sequence of server address 
    bcopy((char *)receiver->h_addr, (char *)&receiverAddress.sin_addr.s_addr, receiver->h_length);
	// Convert multi-byte integer types from host byte order to network byte order 
	receiverAddress.sin_port = htons(dataPort);

    status = connect(dataSocket, (struct sockaddr *)&receiverAddress, sizeof(struct sockaddr));
    if (status < 0)
    {
        printf("   ! Connection Error: Could not establish data connection with %s:%d\n", client, dataPort);
        return -1;
    }

    if (commandType == 1)
    {
        listFilesCmd(dataSocket);
    }
    else
    {
        printf("I'm a teapot.");
    }

    free(args);

    return 0;
};

int startup(int port)
{
    /**
	 * This function takes the port number, initializes a server socket, and returns the active socket file descriptor
	 */
    struct sockaddr_in serverAddress;
    int sock;       // socket file descriptor
    int status;     // generic error response holder
    int optVal = 1; // for socket options

    // Initialize the file descriptor of a new socket [1]
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

    // this socket option allows other sockets to bind on this port [1]
    status = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int));
    if (status < 0)
    {
        fprintf(stderr, "Socket Error: Error initializing socket reusability.\n");
        exit(1);
    }

    // this is where we bind our socket to the local port [1]
    status = bind(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (status < 0)
    {
        fprintf(stderr, "Socket Error: Error binding host to port %d.\n", port);
        exit(1);
    }

    // now we're actually a server because we're listening to connections (arbitrarily set to 5) [1]
    status = listen(sock, 5);
    if (status < 0)
    {
        fprintf(stderr, "Socket Error: Error Setting Up Socket Listener.\n");
        exit(1);
    }

    printf("\nFile Transfer Server Listening on Port %d...\n", port);

    return sock;
};

int shutDown();

int main(int argc, char **argv)
{
    int portNumber;                     // port which we use to start server on
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
    if (validatePort(argv[1]) != 0)
    {
        fprintf(stderr, "Error: Port Number must be an integer between 1024 and 65535 and not 30021 or 30020, you passed: '%s'\n", argv[1]);
        exit(1);
    }
    // after validation, read in port for real
    portNumber = atoi(argv[1]);
    // build a listening controlSocket with the startup function
    controlSocket = startup(portNumber);

    while (1)
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
            printf("\n + Connection from %s\n\n", clientHost);
        }
        else
        {
            fprintf(stderr, "   ! Connection Error: There was an error reading hostname.\n");
        }

        status = handleRequest(activeSocket, clientHost);
        if (status < 0)
        {
            fprintf(stderr, "   ! Connection Error: There was an error handling the connection.\n");
        }

        // make sure to cleanup
        close(activeSocket);
        printf("\n - Connection to %s closed\n", clientHost);
    }

    // make sure to cleanup
    close(controlSocket);

    exit(0);
}
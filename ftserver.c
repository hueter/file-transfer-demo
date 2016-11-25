/**
 * Michael Hueter
 * CS 372 Networking Fall 2016
 * Program 2 - File Transfer (Server Component)
 * 27 November 2016
 * References: [1] http://beej.us/guide/bgnet/
 *			   [2] Advanced Programming in the UNIX Environment, 3rd Edition by W. Richard Stevens
 *			   [3] http://stackoverflow.com/questions/4072190/check-if-input-is-integer-type-in-c
 *			   
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


 int validatePort(char *port){
	/**
	* This function makes sure the port number from the command line is a valid integer in the correct range
	*  Return 1 if validation error, or 0 if it's OK
	*/
    int portNo = atoi(port);
    if ((isdigit(portNo) != 0) || (portNo < 1024) || portNo > 65535) {
        return 1;
    }

    return 0;
 }


 int main(int argc, char **argv){
	int portNumber;

	if (argc != 2) {
		fprintf(stderr, "Usage: ftserver <portNumber>\n");
		exit(1);
	}

    if (validatePort(argv[1]) != 0) {
        fprintf(stderr, "Error: Port Number must be an integer between 1024 and 65535, not '%s'\n", argv[1]);
        exit(1);
    } else {
        portNumber = atoi(argv[1]);
        printf("File Transfer Server Listening on Port %d", portNumber);
    }

	exit(0);
}
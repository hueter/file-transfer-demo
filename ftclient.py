#!/usr/bin/python

"""
Michael Hueter
CS 372 Networking Fall 2016
Program 2 - File Transfer (Client Component)
27 November 2016
References:
[1] https://docs.python.org/2/howto/sockets.html
[2] http://stackoverflow.com/questions/10019456/usage-of-sys-stdout-flush-method
[3] https://docs.python.org/2/howto/argparse.html
[4] https://docs.python.org/2/library/socketserver.html
"""

import argparse
import socket
import SocketServer
import sys


class TCPDataHandler(SocketServer.BaseRequestHandler):
    """
    The request handler class for our server. [4]

    It is instantiated once per connection to the server, and must
    override the handle() method to implement communication to the
    client.
    """
    command = ""
    host = ""
    port = 0
    filename = ""

    def receive_directories(data=None):
        """
        This function takes a socket and prints directory contents from server to stdout
        """
        print("Receiving directory structure from {0}:{1}".format(this.host, this.port))

        if not data:
            print("Server Error: No directory information received.")
            sys.exit(1)

        sys.stdout.write(data)
        # http://stackoverflow.com/questions/10019456/usage-of-sys-stdout-flush-method
        sys.stdout.flush()

    def receive_file(data=None):
        """
        This function takes a socket and prints directory contents from server to stdout
        """
        print("Receiving file '{0}' from {1}:{2}".format(this.filename, this.host, this.port))

        if not data:
            # if unhandled error occurs
            print("Server Error: No file received.")
            sys.exit(1)
        elif "Error" in data:
            # if file not found error thrown from server
            print data
            sys.exit(1)

        # open the requested filename and write data stream to file (overwrite mode with 'w+')
        with open(this.filename, "w+") as outfile:
            outfile.write(data)

        print("File Transfer Complete.")

    def handle(self):
        # self.request is the TCP socket connected to the client
        if command == "-l":
            this.receive_directories(data=self.request.recv(1024).strip())

        elif command == "-g":
            this.receive_file(data=self.request.recv(1024).strip())

        else:
            print("Invalid command handle triggered.")


def initiate_contact(host=None, port=None):
    """
    Initialize a socket and connect to the server
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.settimeout(4000)  # upping the timeout just in case
    try:
        sock.connect((host, port))
    except:
        print("Failed to connect to ({0}, {1})".format(host, port))
        sys.exit(1)

    print("Connection Established.")
    return sock


def make_request(sock=None, command=None, filename=None, port=None):
    """
    Send a command message to the server
    """
    # full_command string differs based on whether or not there is a filename
    if filename:
        full_command = "{0} {1} {2}".format(command, filename, port)
    else:
        full_command = "{0} {1}".format(command, port)

    print("Issuing Server Command: '{}'".format(full_command))
    sock.send(full_command)


def user_input():
    """
    Using the argparse module, get/validate command-line input from the user [3]
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--server_host", "-sh", help="specify host you want to connect to", type=str, required=True)
    parser.add_argument("--server_port", "-sp", help="specify the server port you want to connect to", type=int, required=True)
    parser.add_argument("--command", "-c", help="specify the command you want to execute on the server: either (-l or -g)", type=str, required=True)
    parser.add_argument("--filename", "-f", help="specify the file you want to get (if issuing a get command)", type=str, required=False)
    parser.add_argument("--data_port", "-dp", help="specify the data port you want to transfer using", type=int, required=True)

    # retrieving each arg; if not required and not passed, will be 'None'
    args = parser.parse_args()
    server_host = args.server_host
    server_port = args.server_port
    command = args.command
    filename = args.filename
    data_port = args.data_port

    # --- Client Input Validation

    if command not in ["-l", "-g"]:
        print("Argument Error: You didn't issue a correct command. The options are '-l' or '-g' and you entered '{}'".format(command))
        sys.exit(1)

    if (args.filename and args.command != "-g"):
        print("Argument Error: If you pass a file you must also pass a GET -g command.")
        sys.exit(1)

    for port in [server_port, data_port]:
        if (port < 1024 or port > 65535 or port == 30020 or port == 30021):
            print("Argument Error: Invalid Port Number. Ports must be between 1024 and 65535 and you may not use 30020 or 30021. You passed '{}'".format(port))
            sys.exit(1)

    return (server_host, server_port, command, filename, data_port)


def setup_data_connection(host=None, port=None, command=None, filename=None):
    """
    This function takes a host and (data) port and sets up a TCPServer data connection
    with the host to begin accepting files [4]
    """
    TCPDataHandler.host = host
    TCPDataHandler.port = port
    TCPDataHandler.command = command
    TCPDataHandler.filename = filename
    data_connection = SocketServer.TCPServer((host, port), TCPDataHandler)

    return data_connection


if __name__ == "__main__":
    """
    This is the main function, aka entrypoint to the whole script
    """
    # unpack validated command line arguments
    server_host, server_port, command, filename, data_port = user_input()
    # establish a TCP connection
    sock = initiate_contact(host=server_host, port=server_port)
    # send the server the formatted command
    make_request(sock=sock, command=command, filename=filename, port=server_port)
    # setup data connection with SocketServer class
    data_connection = setup_data_connection(host=server_host, port=data_port, command=command, filename=filename)
    # cleanup
    data_connection.server_close()
    sock.close()

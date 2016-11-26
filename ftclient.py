#!/usr/bin/python

"""
Michael Hueter
CS 372 Networking Fall 2016
Program 2 - Client Component
27 November 2016
References:
[1] https://docs.python.org/2/howto/sockets.html
"""

import argparse
import socket
import getopt
import sys


def setup_socket_and_connect(host=None, port=None):
    """
    Initialize a socket and connect to server
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


def user_input():
    """
    Using the argparse module, get/validate command-line input from the user
    """
    parser = argparse.ArgumentParser()
    parser.add_argument("--server_host", "-sh", help="specify host you want to connect to",
                        type=str, required=True)
    parser.add_argument("--server_port", "-sp", help="specify the server port you want to connect to",
                        type=int, required=True)
    parser.add_argument("--command", "-c", help="specify the command you want to execute on the server: either (-l or -g)",
                        type=str, required=True)
    parser.add_argument("--file", "-f", help="specify the file you want to get (if issuing a get command)",
                        type=str, required=False)
    parser.add_argument("--data_port", "-dp", help="specify the data port you want to transfer using",
                        type=int, required=True)
    args = parser.parse_args()

    server_host = args.server_host
    server_port = args.server_port
    command = args.command
    file = args.file
    data_port = args.data_port

    # --- Client Input Validation

    if command not in ["-l", "-g"]:
        print("Argument Error: You didn't issue a correct command. The options are '-l' or '-g' and you entered '{}'".format(command))
        sys.exit(1)

    if (args.file and args.command != "-g"):
        print("Argument Error: If you pass a file you must also pass a GET -g command.")
        sys.exit(1)

    for port in [server_port, data_port]:
        if (port < 1024 or port > 65535 or port == 30020 or port == 30021):
            print("Argument Error: Invalid Port Number. Ports must be between 1024 and 65535 and you may not use 30020-30021. You passed '{}'".format(port))
            sys.exit(1)

    sock = setup_socket_and_connect(host=server_host, port=server_port)


if __name__ == "__main__":
    """
    This is the main function, aka entrypoint to the whole script
    """
    # call user input to get started
    user_input()

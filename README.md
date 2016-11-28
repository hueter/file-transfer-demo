# CS 372 Intro to Computer Networking | Project 2 - File Transfer
Design and implement a simple file transfer system, i.e., create a file transfer server and a file transfer client.

## Compile / Run Instructions
The server is in C and the client is in Python 2.7

### Server
To compile the server, run:

`gcc -Wall -o ftserver ftserver.c`

In the root directory. Then execute the server as follows: `./ftserver <portNumber>`.

**Note: I was unable to implement immediate SIGTERM handling**. So when you Ctrl-C to exit the server, the server will shut down gracefully AFTER the next client request. This is a very annoying problem but it is due to the `accept()` call blocking everything, including the SIGTERM handler.

### Client
To run the client, you have to pass the keyword arguments as follows:

`python ftclient.py --server_host=<server_host> --server_port=<server_port> --command=<command> --file=<file> --data_port=<data_port>`

**Arguments List:**

- `--server_host` or `-sh`
  - specify host you want to connect to
  - required
- `--server_port`or `-sp`
  - specify the server port you want to connect to
  - required
- `--command` or `-c`
  - specify the command you want to execute on the server: either (-l or -g)`
  - required
- `--filename` or `-f`
  - specify the file you want to get (if issuing a get command)`
  - required with `-g`
- `--data_port` or `-dp`
  - specify the data port you want to transfer using
  - required

_Note: The `--file` argument only goes with `-g` command._
 
 #### List Directory Example:
 `python ftclient.py --server_host=flip1.engr.oregonstate.edu --server_port=50010 --command=-l --data_port=50011`

 #### Get File Example:
 `python ftclient.py --server_host=flip1.engr.oregonstate.edu --server_port=50011 --command=-g --file=binny.bin --data_port=50011`

---
 ## Extra Credit
  - My _Get File_ command `-g` works with non-text files. For example, transfer the included binary `binny.bin`.
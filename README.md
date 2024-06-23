## Printable Character Counter (PCC) Client-Server Application

## Overview

This client-server application facilitates the computation of printable characters in a file using a TCP connection. The server listens for connections, processes file data from clients, computes printable characters, and sends results back. The client connects to the server, sends file data, and retrieves results.

## Installation

1. **Clone the repository**:

   git clone <repository-url>
   cd pcc-client-server

2. **Compile the Server**:

   cd server
   gcc -o server server.c -pthread

3. **Compile the Client**:

   cd ../client
   gcc -o client client.c

##### Usage

1. **Start the Server**:

   cd server
   ./server <port>

   Replace `<port>` with the desired port number (e.g., 12345).

2. **Run the Client**:

   cd client
   ./client <server-ip> <server-port> <file-path>

   Replace `<server-ip>` with the server's IP address, `<server-port>` with the server's port number, and `<file-path>` with the path to the file you want to send.

3. **Expected Output**:

   The client will display the number of printable characters computed by the server based on the contents of the file.

## Features

- **Error Handling**: Comprehensive error handling for file operations, socket connections, and data transfer.
- **Signal Handling**: Server gracefully handles SIGINT to print statistics before termination.
- **Protocol**: Defines a clear protocol for data exchange between client and server using TCP/IP.

## (This assignment is part of the Operating Systems course at Tel Aviv University).

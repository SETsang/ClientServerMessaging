Welcome to my Client Server Messaging application! I wrote this program in C to demonstrate UDP (User Datagram Protocol), which is a connectionless protocol. 

How to run: 

Prerequisite: Have or create a file that you want to send.

Run these commands in this order to run the program
1. gcc client.c -o client
2. gcc server.c -o server
3. pick a port between 1024 and 65535
4. run ./server <port number> 
5. In a seperate window, run ./client <port number> <127.0.0.1> <source file> <destination file>

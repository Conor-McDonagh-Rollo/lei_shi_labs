
# Networking Project

A tag game made in sfml and winsock2. Server/ contains a single cpp file that will handle communications between the clients, using multithreading to asynchronously handle the players without using non-blocking sockets. Client/ contains a single cpp file that will handle the rendering and input as well as asynchronously handling sending and receiving information to/from the server.




## Compiling

You can compile this project with make, however you first need to direct the makefile to where your includes and libs are for sfml. This project uses a MingGW64 version of sfml which isn't normally available. To get that version of sfml you will have to clone sfml and compile it with MingGW64. Then link to those binaries in your path, and the include and lib in this makefile.

```bash
  make
```

if make only opens the server and not 3 client windows, you need to use MingGW64 to run make. Please do not use a different terminal.
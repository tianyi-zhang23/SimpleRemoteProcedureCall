# SimpleRemoteProcedureCall
A simple remote procedure call service written in C. It is intended for use on Linux, but probably works on other \*nix operating systems too.
mainClient.c and mainServer.c is a demonstration of how this library works. After compiling, try client and server by using
```bash
./backend ADDRESS PORT
```
for the server and
```bash
./frontend SERVER_ADDRESS SERVER_PORT
```
for the client.

//
// Created by tianyizhang on 2020-09-21.
//

#ifndef RPCSERV_RPCSERV_H
#define RPCSERV_RPCSERV_H

#endif //RPCSERV_RPCSERV_H

#include <stdio.h>
#include <stdint.h>
#include <netinet/in.h>

enum FunctionType {intintToint, intToint, floatfloatTofloat, intTouint64_t};

union FunctionPointer
{
    int (*intintToint)(int,int);
    int (*intToint)(int);
    float (*floatfloatTofloat)(float,float);
    uint64_t (*intTouint64_t)(int);
};

typedef struct {
    enum FunctionType  type;
    union FunctionPointer ptr;
} callback_t;

typedef struct FuncListNode funcListNode;
struct FuncListNode
{
    char* funcName;
    callback_t callback;
    funcListNode * next;

};

typedef struct
{
    char* hostName;
    int portNumber;
    funcListNode* headOfList;
    funcListNode* endOfList;
    int serverSocket;
    struct sockaddr_in serverAddress;

} rpc_t;

typedef struct {
    int clientSocket;
    struct sockaddr_in serverAddress;
    struct hostent *server;
} rpcServer_t;
//Server side functions
rpc_t *RPC_Init(char *host, int port); //initialize the RPC struct
void RPC_Register(rpc_t *r, char *name, callback_t fn); //register a callback function. Do this before StartAccepting(), since the process will be blocked
void RPC_StartAccepting(rpc_t *r); //start the server. Allows client connection.

//Client side functions
rpcServer_t *RPC_Connect(char *name, int port); //connect to a server
void RPC_ShutdownServer(rpcServer_t* r); //shut the server down
void RPC_CloseClient(rpcServer_t* r); //close the client, but does not shutdown the server
void RPC_Call(rpcServer_t *r, char *name);





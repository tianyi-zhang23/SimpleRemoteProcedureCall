//
// Created by tianyizhang on 2020-09-23.
//

#include "RPCServ.h"
#include <string.h>
#include <stdlib.h>

int main(int argc,char* argv[])
{
    char* serverAddr = argv[1];
    int serverPort =atoi(argv[2]);
    //printf("Connecting to the server at %s:%d\n",serverAddr,serverPort);
    rpcServer_t* backend = RPC_Connect(serverAddr,serverPort);

    while(1)
    {
        printf(">> ");
        //get userInput
        char* userInput = NULL;
        size_t lineLength = 0;
        ssize_t lineSize = 0;
        lineSize = getline(&userInput,&lineLength,stdin);
        //Parse user input
        //The user input will be partially parsed. Only "exit" and "shutdown" commands will be parsed. Parsing of function calling and illegal inputs
        //are to be done by the server.
        if(strcmp(userInput,"exit\n")==0)
        {
            RPC_CloseClient(backend);
            exit(0);
        }
        else if(strcmp(userInput,"shutdown\n")==0 || strcmp(userInput,"quit\n")==0)
        {
            RPC_ShutdownServer(backend); //ShutdownServer will also callCloseClient().
            exit(0);
        }
        else
        {RPC_Call(backend,userInput);}

    }
}

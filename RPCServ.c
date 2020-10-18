#include "RPCServ.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
//#include <bits/socket_type.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <netdb.h>
#include <inttypes.h>
#include <math.h>

void freeRPC_t(rpc_t* r) //this frees a rpc_t. The reason why a function is needed is that the struct makes use of dynamically allocated linked list.
{
    funcListNode* cur = r->headOfList;
    while(cur!=NULL)
    {
        funcListNode* prev = cur;
        cur = cur->next;
        free(prev);
    }
}
rpc_t *RPC_Init(char *host, int port)
{
    rpc_t* newRPC = malloc(sizeof(rpc_t)); //make a new RPC struct
    newRPC->hostName = malloc(sizeof(char)*strlen(host));
    strcpy(newRPC->hostName,host); //assign the hostname to the RPC struct
    newRPC->portNumber = port;
    newRPC->headOfList = NULL; //assign NULL to the headOfList to the function list
    newRPC->endOfList = NULL;

    newRPC->serverSocket = socket(AF_INET,SOCK_STREAM,0);
    if (newRPC->serverSocket<0)
    {
        perror("Cannot open socket.");
        exit(1);
    }
    memset(&newRPC->serverAddress,0, sizeof(newRPC->serverAddress)); //empty serverAddress struct
    newRPC->portNumber = port;

    //set of serverAddress
    newRPC->serverAddress.sin_family = AF_INET;
    newRPC->serverAddress.sin_port = htons(newRPC->portNumber);
    inet_pton(AF_INET, host, &(newRPC->serverAddress.sin_addr.s_addr));

    if(bind(newRPC->serverSocket,(struct sockaddr *)&newRPC->serverAddress,sizeof(newRPC->serverAddress))<0) //bind socket to address
    {
        perror("bind() failed");
        exit(1);
    }

    if(listen(newRPC->serverSocket,5)<0) //listen for incoming connections
    {
        perror("listen() failed");
        exit(1);
    }
    return newRPC;
}

void RPC_Register(rpc_t *r, char *name, callback_t fn) //register a callback function
{
    //create the new node
    funcListNode* newNode = malloc(sizeof(funcListNode)); //make a node to store info of this new function
    newNode->funcName = malloc(sizeof(char)*strlen(name));
    strcpy(newNode->funcName,name); //put the name of the new function into the node
    newNode->callback = fn;
    newNode->next = NULL;

    //if the headOfList in the rpc_t is NULL, then this is the first entry. Make newNode both the head node and the end node.
    if(r->headOfList==NULL)
    {
        r->headOfList = newNode;
        r->endOfList = newNode;
    }
    else
    {
        r->endOfList->next = newNode; //if the list is not empty, the first make newNode the next of endOfList
        r->endOfList = newNode; //then make newNode the endOfList
    }
}

char* executeCallBack(callback_t* c,char* args) //execute a callback and return a pointer to its return value
{
    enum FunctionType type = c->type;
    int returnValInt;
    float returnValFloat;
    uint64_t returnValUint64_t;
    char* returnValAsString = malloc(sizeof(char)*256); //needs to be freed by the caller.
    char arg1[32];
    char arg2[32];

    switch(type)
    {
        case intintToint:
            sscanf(args,"%s %s\n",arg1,arg2);
            returnValInt = c->ptr.intintToint(atoi(arg1),atoi(arg2)); //call the actual function with the specified arguments
            sprintf(returnValAsString,"%d",returnValInt);//convert returned int to string so that it can be communicated to the client. Needs to be freed later.
            return returnValAsString; //the caller must free this.
        case intToint:
            sscanf(args,"%s\n",arg1);
            returnValInt = c->ptr.intToint(atoi(arg1));
            sprintf(returnValAsString,"%d",returnValInt);
            return returnValAsString;
        case floatfloatTofloat:
            sscanf(args,"%s %s\n",arg1,arg2);
            returnValFloat = c->ptr.floatfloatTofloat(atof(arg1),atof(arg2));
	    if(returnValFloat != INFINITY)
            	sprintf(returnValAsString,"%f",returnValFloat);
            return returnValAsString;
        case intTouint64_t:
            sscanf(args,"%s\n",arg1);
            returnValUint64_t = c->ptr.intTouint64_t(atoi(arg1));
            sprintf(returnValAsString,"%" PRIu64,returnValUint64_t);
            return returnValAsString;
    }
}

char* parseAndExecuteClientRequest(rpc_t *r, char* command)
{
    char funcName[32];
    int pos;
    sscanf(command,"%s%n",funcName,&pos);
    char* returnValueAsString = NULL;
    funcListNode* cur = r->headOfList;
    int matchFlag = 0;
    while(cur!=NULL && !matchFlag) //for each node in the function registry (traverse the list)
    {
        if( strcmp(cur->funcName,funcName) == 0 )//if the name of the node in the function registry is equal to the one desired by the client
        {
            returnValueAsString = executeCallBack(&cur->callback,command+pos+1);
            matchFlag = 1;
        }
        cur = cur->next;
    }
    if(!matchFlag)  //if the while finishes and there is still no matching function, there is no such function registered.
    {
        returnValueAsString = malloc(64* sizeof(char));
        sprintf(returnValueAsString,"Error: Command \"%s\" not found",funcName);
    }
    return returnValueAsString; //the caller must free this. It's dynamically allocated whether the function is found or not.

}

void RPC_StartAccepting(rpc_t *r)
{
    while(1)
    {
        struct sockaddr_in clientAddress;
        unsigned int clientAddrSize = sizeof(clientAddress);
        int clientSocket = accept(r->serverSocket,(struct sockaddr*) &clientAddress,&clientAddrSize); //blocks until a connection comes in
        printf("Parent: a client has been accepted.\n");
        //this portion of the code checks if a child has already signaled a termination of the server.
        int returnValue;
        int shutdownFlag =0;

        int res = waitpid(-1,&returnValue,WNOHANG); //get the return value of a child, if there is one that returns

        while(res>0) //for each child that has returned, check if it has returned 123 (shutdown signal). if it did, set shutdownFlag to be true;
        {
            if(WEXITSTATUS(returnValue)==123)
            {
                shutdownFlag=1;
                break;
            }
            res = waitpid(-1,&returnValue,WNOHANG);
        }

        if(shutdownFlag) //if a child has indeed returned and the return value is 123, which for me signals terminating the server
        {
            char* shutdownMessage = "The previous client has initiated a shutdown which will now be completed. The server will be shutdown.";
            write(clientSocket,shutdownMessage,strlen(shutdownMessage));

            close(clientSocket);
            close(r->serverSocket); //close the sockets
            freeRPC_t(r); //free the memory occupied by r since the server programme is about to terminate
            int status=0;
            printf("Parent: The server is about to shutdown because a shutdown command has previously been issued by a client. Now waiting for the other clients to finish.\n");
            while (wait(&status)>=0); //wait for all children, until wait returns -1 (error, no more child)
            printf("Parent: All clients have finished. Shutting down now.\n");
            exit(0); //terminate the parent process
        }

        //now that an exit has not been signaled, proceed to process this new connection.
        pid_t pid = fork();
        if(pid<0) //error
        {
            perror("fork() failed");
            exit(1);
        }
        else if (pid==0) //child
        {
            printf("Child %d: Now in charge of this client.\n",getpid());
            close(r->serverSocket); //close the listening socket, keeps the accepted socket
            dup2(clientSocket,STDERR_FILENO); //redirect standard error to the socket so the user sees errors.
            int numOfCharRW;
            char buffer[256];
            memset(buffer,0,256); //clear buffer
            numOfCharRW = read(clientSocket,buffer,255); //read the next user input
            while(numOfCharRW > 0) //while reading is successful (client not disconnected)
            {

                if (strcmp(buffer,"kill")==0) //if client wants the server t
                 {
                    printf("Child %d: The client has issued a shutdown command. This child exiting.",getpid());
                    exit(123);
                 }
                else{
                    char* returnValueInChar = parseAndExecuteClientRequest(r,buffer); //execute the function desired
                    write(clientSocket,returnValueInChar,strlen(returnValueInChar)); //write the response to the client
                    free(returnValueInChar); //free the result string returned by parseAndExecuteClientRequest
                }
                memset(buffer,0,256); //clear buffer
                numOfCharRW = read(clientSocket,buffer,255); //read the next user input
                if(numOfCharRW <= 0)
                {
                    break; //if read fails (returns nonpositive number), then the client has disconnected. The program (child) shall exit.
                }
            }
            //on the outside of the while loop, the client has disconnected.
            //now delete everything and exit program.
            freeRPC_t(r);
            printf("Child %d: a client has disconnected. This child exiting.\n",getpid());
            exit(0);
        }
    }
}

//below are the functions for client
rpcServer_t *RPC_Connect(char *name, int port) //connect to a server
{

    rpcServer_t * serverInfo = malloc(sizeof(rpcServer_t));
    serverInfo->server = gethostbyname(name);
    serverInfo->clientSocket = socket(AF_INET, SOCK_STREAM,0);

    if(serverInfo->server == NULL) //if host name/IP cannot be resolved/found
    {
        perror("gethostname() failed. There is probably no such host.");
        exit(1);
    }

    //set up serverAddress
    memset(&serverInfo->serverAddress,0,sizeof(serverInfo->serverAddress)); //zero out the serverAddress
    serverInfo->serverAddress.sin_family = AF_INET;
    memmove(&serverInfo->serverAddress.sin_addr.s_addr,serverInfo->server->h_addr,serverInfo->server->h_length); //get server IP address from hostent serverInfo->server
    serverInfo->serverAddress.sin_port = htons(port);

    //connect
    if(connect(serverInfo->clientSocket, (struct sockaddr *)&(serverInfo->serverAddress), sizeof(serverInfo->serverAddress)) < 0 )
    {
        perror("error in connect()");
        exit(1);
    }
    return serverInfo;
}

void RPC_Call(rpcServer_t *r, char *name) //ask the server to execute the function "name"
{
    char readBuffer[256];
    memset(readBuffer,0,256);
    write(r->clientSocket,name,strlen(name));
    read(r->clientSocket,readBuffer,255);
    printf("%s\n",readBuffer);
    fflush(stdout);
}


void RPC_ShutdownServer(rpcServer_t* r)
{
    write(r->clientSocket,"kill",strlen("kill"));
    RPC_CloseClient(r);
}

void RPC_CloseClient(rpcServer_t* r)
{
    close(r->clientSocket);
    free(r);
}






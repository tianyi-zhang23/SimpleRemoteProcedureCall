#include "RPCServ.h"
#include <math.h>
#include <unistd.h>
#include <stdlib.h>

int addInts(int a, int b){return a+b;}
int multiplyInts(int a, int b){return a*b;};
int mySleep(int seconds){return sleep(seconds);}
float divideFloats(float a, float b)
{
    if (b==0)
    {
        fprintf(stderr,"Error: Division by zero");
        return INFINITY;
    }
    else
    {
        return a/b;
    }
}
uint64_t factorial(int x)
{
    return x == 0 ? 1 : x * factorial(x - 1);
}
int main(int argc,char *argv[])
{
    char* addr = argv[1];
    int port =atoi(argv[2]);

    printf("Starting server at %s:%d\n",addr,port);
    rpc_t* serv = RPC_Init(addr,port); //Initialize an RPC
    printf("Server initiated.\n");
    //register all the functions
    union FunctionPointer addIntsPtr = { .intintToint = &addInts };
    callback_t addInts = { .ptr = addIntsPtr, .type = intintToint};
    RPC_Register(serv,"add",addInts);

    union FunctionPointer multiplyIntsPtr = {.intintToint = &multiplyInts};
    callback_t multiplyInts = { .ptr = multiplyIntsPtr, .type = intintToint};
    RPC_Register(serv,"multiply",multiplyInts);

    union FunctionPointer divideFloatsPtr = {.floatfloatTofloat=&divideFloats};
    callback_t divideFloats = { .ptr = divideFloatsPtr, .type = floatfloatTofloat};
    RPC_Register(serv,"divide",divideFloats);

    union FunctionPointer factorialPtr = {.intTouint64_t=&factorial};
    callback_t factorial = { .ptr = factorialPtr, .type = intTouint64_t};
    RPC_Register(serv,"factorial",factorial);

    union FunctionPointer sleepPtr = {.intToint = &mySleep};
    callback_t sleep = { .ptr = sleepPtr, .type = intToint};
    RPC_Register(serv,"sleep",sleep);

    printf("The following functions: add, multiply, factorial and sleep have been registered with RPC. The server is accepting clients now.\n");
    RPC_StartAccepting(serv);
}

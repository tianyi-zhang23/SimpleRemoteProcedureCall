rpc: frontend backend
frontend: mainClient.c RPCServ.c RPCServ.h
	gcc mainClient.c RPCServ.c -o frontend

backend: mainServer.c RPCServ.c RPCServ.h
	gcc mainServer.c RPCServ.c -o backend

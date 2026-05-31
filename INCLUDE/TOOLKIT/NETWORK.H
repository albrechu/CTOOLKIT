#pragma once

#include <TOOLKIT/TYPES.H>

#ifdef __cplusplus
extern "C" {
#endif

typedef uintptr_t SOCKET;
typedef CHAR      IPV4[16];
typedef CHAR      IPV6[46];
typedef U16       PORT, *PPORT;

typedef enum SOCKETTYPE_
{
	SOCKETTYPE_UDP,
	SOCKETTYPE_TCP,
} SOCKETTYPE;

SOCKET LoadSocket(SOCKETTYPE type);
VOID   FreeSocket(SOCKET socket);
I64    SocketConnect(SOCKET socket, IPV4 ip, PORT port);
I64    SocketListen(SOCKET socket, PORT port, I32 backlog);
SOCKET SocketAccept(SOCKET server);
U64    SocketSend(SOCKET socket, DV data);
DV  SocketReceive(SOCKET socket, DV buffer);
U64    SocketSendTo(SOCKET socket, IPV4 ip, PORT port, DV data);
DV  SocketReceiveFrom(SOCKET socket, IPV4 ip_out, DV buffer, PPORT port);
I64    SocketBlock(SOCKET socket, BOOL blocking);
BOOL   SocketIsOpen(SOCKET socket);

#ifdef __cplusplus
}
#endif

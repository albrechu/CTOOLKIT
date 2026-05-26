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
U64    SocketSend(SOCKET socket, FDATA data);
FDATA  SocketReceive(SOCKET socket, FDATA buffer);
U64    SocketSendTo(SOCKET socket, IPV4 ip, PORT port, FDATA data);
FDATA  SocketReceiveFrom(SOCKET socket, IPV4 ip_out, FDATA buffer, PPORT port);
I64    SocketBlock(SOCKET socket, BOOL blocking);
BOOL   SocketIsOpen(SOCKET socket);

#ifdef __cplusplus
}
#endif

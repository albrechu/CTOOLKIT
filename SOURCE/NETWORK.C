#include <TOOLKIT/NETWORK.H>

#include <assert.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif

static BOOL InitPlatform(int sockets)
{
    static I32 sockets_count = 0;
    int initialize = sockets > 0 and sockets_count == 0;
    sockets_count += sockets;
    int shutdown = sockets_count == 0;
#if defined(_WIN32)
    if (initialize)
    {
        WSADATA wsa;
        int ec = WSAStartup(MAKEWORD(2, 2), &wsa);
        switch (ec)
        {
        case WSASYSNOTREADY: return false;
        case WSAVERNOTSUPPORTED: return false;
        case WSAEINPROGRESS: return false;
        case WSAEPROCLIM: return false;
        case WSAEFAULT: return false;
        default: break;
        }
    }
    if (shutdown)
    {
        WSACleanup();
    }
#endif
    return true;
}
SOCKET LoadSocket(SOCKETTYPE type)
{
    if (not InitPlatform(1))
        return ~0;
    int socket_type;
    switch (type)
    {
    case SOCKETTYPE_UDP: socket_type = SOCK_DGRAM; break;
    case SOCKETTYPE_TCP: socket_type = SOCK_STREAM; break;
    default: unreachable();
    }
#if defined(_WIN32)
    return socket(AF_INET, socket_type, 0);
#else
    return (SOCKET)socket(AF_INET, socket_type, 0);
#endif
}
VOID FreeSocket(SOCKET socket)
{
#if defined(_WIN32)
    closesocket((SOCKET)socket);
#else
    close((int)socket);
#endif
    _ = InitPlatform(-1);
}
I64 SocketConnect(SOCKET socket, IPV4 ip, PORT port)
{
    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    return connect((int)socket, (struct sockaddr *)&addr, sizeof(addr));
}
I64 SocketListen(SOCKET socket, PORT port, I32 backlog)
{
    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind((int)socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        return -1;
    return listen((int)socket, backlog);
}
SOCKET SocketAccept(SOCKET server)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
#if defined(_WIN32)
    SOCKET socket = accept((SOCKET)server, (struct sockaddr *)&addr, &len);
    if (socket != INVALID_SOCKET)
    {
        if (not InitPlatform(1))
            return ~0;
    }
    return (SOCKET)socket;
#else
    int socket = accept((int)server, (struct sockaddr *)&addr, &len);
    if (socket >= 0)
        if (not InitPlatform(1))
            return ~0;
    return (SOCK)socket;
#endif
}
U64 SocketSend(SOCKET socket, FDATA data)
{
    int bytes = send(socket, data.data, (int)data.size, 0);
    if (bytes <= 0)
        return 0;
    return (U64)bytes;
}
FDATA SocketReceive(SOCKET socket, FDATA buffer)
{
    int bytes = recv(socket, buffer.data, (int)buffer.size, 0);
    if (bytes <= 0)
        return FDATA_INVALID;
    return fdata(buffer.data, (U64)bytes);
}
U64 SocketSendTo(SOCKET socket, IPV4 ip, PORT port, FDATA data)
{
    assert(SocketIsOpen(socket));

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
#ifdef _WIN32
    inet_pton(AF_INET, ip, &addr.sin_addr);
#else
    inet_pton(AF_INET, ip, &addr.sin_addr.s_addr);
#endif
    int sent = sendto(socket, (CSTR)data.data, (int)data.size, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (sent <= 0)
        return 0;
    return (U64)sent;
}
FDATA SocketReceiveFrom(SOCKET socket, IPV4 ip_out, FDATA buffer, PPORT port)
{
    assert(SocketIsOpen(socket));

    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int received = recvfrom(socket, (PCHAR)buffer.data, (int)buffer.size, 0, (struct sockaddr *)&addr, &addrlen);
    if (received < 0)
        return FDATA_INVALID;

    if (ip_out)
    {
        inet_ntop(AF_INET, &addr.sin_addr, ip_out, 16);
        ip_out[15] = '\0';
    }
    if (port)
        *port = ntohs(addr.sin_port);
    return fdata(buffer.data, (U64)received);
}
I64 SocketBlock(SOCKET socket, BOOL blocking)
{
#if defined(_WIN32)
    u_long mode = blocking ? false : true;
    return ioctlsocket((SOCKET)socket, FIONBIO, &mode);
#else
    int flags = fcntl((int)socket, F_GETFL, 0);
    if (!blocking)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;
    return fcntl((int)socket, F_SETFL, flags);
#endif
}
BOOL SocketIsOpen(SOCKET socket)
{
#ifdef _WIN32
    if (socket == INVALID_SOCKET)
        return false;

    u_long mode = 1; // Non-blocking temporarily
    ioctlsocket(socket, FIONBIO, &mode);

    CHAR buf;
    int r = recv(socket, &buf, 1, MSG_PEEK);
    mode = 0; // Restore blocking
    ioctlsocket(socket, FIONBIO, &mode);
    if (r > 0)
        return true;
    if (r == 0)
        return false;
    int err = WSAGetLastError();
    if (err == WSAEWOULDBLOCK)
        return true;
    return false;
#else
    if (socket < 0)
        return false;

    CHAR buf;
    int r = recv(socket, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
    if (r > 0)
        return true;
    if (r == 0)
        return false;
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        return true;
    return false;
#endif
}

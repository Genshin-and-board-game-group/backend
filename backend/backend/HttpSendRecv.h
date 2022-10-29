#pragma once
#include "common.h"
#include "HttpSendRecv.h"

typedef struct _CONNECTION_INFO
{
    WEB_SOCKET_HANDLE hWebSock;
    HTTP_REQUEST_ID RequestID;
    LONG64 volatile RefCnt;

} CONNECTION_INFO, * PCONNECTION_INFO;

typedef struct _WEBSOCK_SENDBUF WEBSOCK_SEND_BUF, * PWEBSOCK_SEND_BUF;
typedef VOID(*WEBSOCK_SEND_CALLBACK)(PCONNECTION_INFO pConnInfo, PWEBSOCK_SEND_BUF WebsockSendBuf);

typedef struct _WEBSOCK_SENDBUF
{
    WEB_SOCKET_BUFFER WebsockBuf;
    WEBSOCK_SEND_CALLBACK Callback;
}WEBSOCK_SEND_BUF, *PWEBSOCK_SEND_BUF;

BOOL StartHTTPServer(DWORD RequestCount);

VOID StopHTTPServer(VOID);

BOOL WebsockSendMessage(_In_ PCONNECTION_INFO pConnInfo, _In_ PWEBSOCK_SEND_BUF pWebsockSendBuf);

BOOL WebsockDisconnect(_In_ PCONNECTION_INFO pConnInfo);

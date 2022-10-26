#pragma once
#include "common.h"
#include "HttpSendRecv.h"

typedef struct _WEBSOCK_CONNECTION_INFO
{
    WEB_SOCKET_HANDLE hWebSock;
    HTTP_REQUEST_ID RequestID;
    LONG64 volatile RefCnt;

    PVOID UserContext; // save any information you want here.
} WEBSOCK_CONNECTION_INFO, * PWEBSOCK_CONNECTION_INFO;

BOOL StartHTTPServer(DWORD RequestCount);

VOID StopHTTPServer(VOID);

BOOL WebsockSendMessage(_In_ PWEBSOCK_CONNECTION_INFO pWebsockConnInfo, _In_ PWEB_SOCKET_BUFFER pBuffer);

BOOL WebsockDisconnect(_In_ PWEBSOCK_CONNECTION_INFO pWebsockConnInfo);

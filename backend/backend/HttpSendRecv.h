#pragma once
#include "common.h"
#include "HttpSendRecv.h"

typedef struct _CONNECTION_INFO
{
    WEB_SOCKET_HANDLE hWebSock;
    HTTP_REQUEST_ID RequestID;
    LONG64 volatile RefCnt;

} CONNECTION_INFO, * PCONNECTION_INFO;

BOOL StartHTTPServer(DWORD RequestCount);

VOID StopHTTPServer(VOID);

BOOL WebsockSendMessage(_In_ PCONNECTION_INFO pConnInfo, _In_ PWEB_SOCKET_BUFFER pBuffer);

BOOL WebsockDisconnect(_In_ PCONNECTION_INFO pConnInfo);

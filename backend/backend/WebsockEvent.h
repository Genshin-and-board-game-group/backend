#pragma once
#include "common.h"
#include "HttpSendRecv.h"

VOID WebsockEventConnect(_In_ PWEBSOCK_CONNECTION_INFO pWebsockConnInfo);

VOID WebsockEventRecv(
    _In_ PWEBSOCK_CONNECTION_INFO pWebsockConnInfo,
    _In_ WEB_SOCKET_BUFFER_TYPE BufferType,
    _In_ PWEB_SOCKET_BUFFER pBuffer);

VOID WebsockEventSendFinish(
    _In_ PWEBSOCK_CONNECTION_INFO pWebsockConnInfo,
    _In_ PWEB_SOCKET_BUFFER pBuffer);

VOID WebsockEventDisconnect(_In_ PWEBSOCK_CONNECTION_INFO pWebsockConnInfo);

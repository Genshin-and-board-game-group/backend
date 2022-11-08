#pragma once
#include "common.h"
#include "HttpSendRecv.h"

VOID WebsockEventConnect(_In_ PCONNECTION_INFO pConnInfo);

VOID WebsockEventRecv(
    _Inout_ PCONNECTION_INFO pConnInfo,
    _In_ WEB_SOCKET_BUFFER_TYPE BufferType,
    _In_ PWEB_SOCKET_BUFFER pBuffer);

VOID WebsockEventDisconnect(_Inout_ PCONNECTION_INFO pConnInfo);

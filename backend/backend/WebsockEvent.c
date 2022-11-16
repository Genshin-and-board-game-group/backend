#include "common.h"
#include "HttpSendRecv.h"
#include "JsonHandler.h"

// functions to receive websocket events.

VOID WebsockEventConnect(_In_ PCONNECTION_INFO pConnInfo)
{
    Log(LOG_INFO, L"a player connected");
}

VOID WebsockEventRecv(
    _Inout_ PCONNECTION_INFO pConnInfo,
    _In_ WEB_SOCKET_BUFFER_TYPE BufferType,
    _In_ PWEB_SOCKET_BUFFER pBuffer)
{
    switch (BufferType)
    {
    case WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE:
    {
        if (!ParseAndDispatchJsonMessage(pConnInfo, pBuffer->Data.pbBuffer, pBuffer->Data.ulBufferLength))
        {
            Log(LOG_ERROR, L"Failed to handle json message. disconnecting...");
            WebsockDisconnect(pConnInfo);
        }
        break;
    }

    case WEB_SOCKET_CLOSE_BUFFER_TYPE:
        Log(LOG_DEBUG, L"Received a close buffer.");
        break;

    case WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE:
    case WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE:
    case WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE:
    case WEB_SOCKET_PING_PONG_BUFFER_TYPE:
    case WEB_SOCKET_UNSOLICITED_PONG_BUFFER_TYPE:
        Log(LOG_ERROR, L"Received an unsupported websocket buffer type.");
        break;
    }
}

VOID WebsockEventDisconnect(_Inout_ PCONNECTION_INFO pConnInfo)
{
    Log(LOG_INFO, L"a player disconnected");
}

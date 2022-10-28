#include "common.h"
#include "HttpSendRecv.h"
#include "JsonHandler.h"

// functions to receive websocket events.

VOID WebsockEventConnect(_In_ PCONNECTION_INFO pConnInfo)
{
    printf("a player connected\n");
}

VOID WebsockEventRecv(
    _In_ PCONNECTION_INFO pConnInfo,
    _In_ WEB_SOCKET_BUFFER_TYPE BufferType,
    _In_ PWEB_SOCKET_BUFFER pBuffer)
{
    switch (BufferType)
    {
    case WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE:
    {
        if (!ParseAndDispatchJsonMessage(pConnInfo, pBuffer->Data.pbBuffer, pBuffer->Data.ulBufferLength))
        {
            printf("Failed to parse or dispatch json message. disconnecting...\n");
            WebsockDisconnect(pConnInfo);
            break;
        }

        /*static WEB_SOCKET_BUFFER buf;
        buf.Data.pbBuffer = "HeLLoWoRlD";
        buf.Data.ulBufferLength = _countof("HeLLoWoRlD") - 1;
        WebsockSendMessage(pConnInfo, &buf);*/

        break;
    }

    case WEB_SOCKET_CLOSE_BUFFER_TYPE:
        printf("Received a close buffer\n");
        break;

    case WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE:
    case WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE:
    case WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE:
    case WEB_SOCKET_PING_PONG_BUFFER_TYPE:
    case WEB_SOCKET_UNSOLICITED_PONG_BUFFER_TYPE:
        printf("Received an unsupported buffer type\n");
        break;
    }
}

VOID WebsockEventSendFinish(
    _In_ PCONNECTION_INFO pConnInfo,
    _In_ PWEB_SOCKET_BUFFER pBuffer)
{
    // Free all resources used when sending here.
}

VOID WebsockEventDisconnect(_In_ PCONNECTION_INFO pConnInfo)
{
    printf("a player disconnected\n");
}

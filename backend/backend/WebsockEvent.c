#include "common.h"
#include "HttpSendRecv.h"

// functions to receive websocket events.

VOID WebsockEventConnect(_In_ PWEBSOCK_CONNECTION_INFO pWebsockConnInfo)
{
    printf("a player connected\n");
}

VOID WebsockEventRecv(
    _In_ PWEBSOCK_CONNECTION_INFO pWebsockConnInfo,
    _In_ WEB_SOCKET_BUFFER_TYPE BufferType,
    _In_ PWEB_SOCKET_BUFFER pBuffer)
{
    switch (BufferType)
    {
    case WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE:

        printf("Got a message: ");
        for (ULONG i = 0; i < pBuffer->Data.ulBufferLength; i++) putchar(pBuffer->Data.pbBuffer[i]);
        printf("\n");

        static WEB_SOCKET_BUFFER buf;
        buf.Data.pbBuffer = "HeLLoWoRlD";
        buf.Data.ulBufferLength = _countof("HeLLoWoRlD") - 1;
        WebsockSendMessage(pWebsockConnInfo, &buf);

        break;

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
    _In_ PWEBSOCK_CONNECTION_INFO pWebsockConnInfo,
    _In_ PWEB_SOCKET_BUFFER pBuffer)
{
    Sleep(0);
}

VOID WebsockEventDisconnect(_In_ PWEBSOCK_CONNECTION_INFO pWebsockConnInfo)
{
    printf("a player disconnected\n");
}

#pragma once
#include "common.h"
#include <Websocket.h>
#include <http.h>
#include "RoomManager.h"

typedef struct _CONNECTION_INFO
{
    WEB_SOCKET_HANDLE hWebSock;
    HTTP_REQUEST_ID RequestID;
    LONG64 volatile RefCnt;

    // Game related information. all rest information below is valid only if pRoom is not NULL
    // all rest index needs to acquire the room's lock in order to modify.
    // (the one who hold's the room's lock also can modify other's index in the same room
    PGAME_ROOM pRoom;
    UINT WaitingIndex; // the index of pRoom->WaitingList field
    UINT PlayingIndex; // the index of pRoom->PlayingList field
} CONNECTION_INFO, * PCONNECTION_INFO;

typedef struct _WEBSOCK_SENDBUF WEBSOCK_SEND_BUF, * PWEBSOCK_SEND_BUF;
typedef VOID(*WEBSOCK_SEND_CALLBACK)(PCONNECTION_INFO pConnInfo, PWEBSOCK_SEND_BUF WebsockSendBuf);

typedef struct _WEBSOCK_SENDBUF
{
    WEB_SOCKET_BUFFER WebsockBuf;
    WEBSOCK_SEND_CALLBACK Callback;
}WEBSOCK_SEND_BUF, *PWEBSOCK_SEND_BUF;

BOOL StartHTTPServer(DWORD RequestCount, LPCWSTR lpListenURL);

VOID StopHTTPServer(VOID);

BOOL WebsockSendMessage(_Inout_ PCONNECTION_INFO pConnInfo, _In_ PWEBSOCK_SEND_BUF pWebsockSendBuf);

BOOL WebsockDisconnect(_In_ PCONNECTION_INFO pConnInfo);

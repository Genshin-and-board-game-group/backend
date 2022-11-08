#pragma once
#include "common.h"

#define ROOM_NUMBER_MIN 10000
#define ROOM_NUMBER_MAX 99999

#define ROOM_PLAYER_MAX 10

#define PLAYER_NICK_MAXLEN 32
#define PLAYER_AVATAR_MAXLEN 32
#define ROOM_PASSWORD_MAXLEN 32

typedef struct _CONNECTION_INFO CONNECTION_INFO, * PCONNECTION_INFO;

typedef struct _PLAYER_INFO
{
    PCONNECTION_INFO pConnInfo;
    UINT GameID;
    BOOL bIsRoomOwner;
    char NickName[PLAYER_NICK_MAXLEN + 1];
    char Avatar[PLAYER_AVATAR_MAXLEN + 1];
}PLAYER_INFO, *PPLAYER_INFO;

typedef struct _GAME_ROOM
{
    UINT RoomNumber;
    LONG64 RefCnt;

    BOOL bGaming; // is game running. (or waiting otherwise)
    UINT IDCount;

    SRWLOCK PlayerListLock; // Visiting / Writing following field needs this lock.

    UINT WaitingCount;
    UINT PlayingCount;
    PLAYER_INFO WaitingList[ROOM_PLAYER_MAX];
    PLAYER_INFO PlayingList[ROOM_PLAYER_MAX]; // Valid only when bGaming == TRUE. pConnInfo could be NULL, In this case, user is offline.

    char Password[ROOM_PASSWORD_MAXLEN + 1];
}GAME_ROOM, * PGAME_ROOM;

VOID InitRoomManager(VOID);

BOOL CreateRoom(_Inout_ PCONNECTION_INFO pConnInfo, _In_z_ const char* NickName, _In_opt_z_ const char* Password);

BOOL JoinRoom(_In_ UINT RoomNum, _Inout_ PCONNECTION_INFO pConnInfo, _In_z_ const char* NickName, _In_opt_z_ const char* Password);

VOID LeaveRoom(_Inout_ PCONNECTION_INFO pConnInfo);

BOOL ChangeAvatar(_Inout_ PCONNECTION_INFO pConnInfo, _In_z_ const char* Avatar);

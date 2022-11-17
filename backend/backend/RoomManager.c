#define _CRT_RAND_S
#include <stdlib.h>
#include <strsafe.h>

#include "common.h"
#include "RoomManager.h"
#include "HttpSendRecv.h"
#include "MessageSender.h"
// This lock must be acquired when creating / deleting / entering / leaving a room
SRWLOCK RoomPoolLock = SRWLOCK_INIT;

#define TOT_ROOM_CNT (ROOM_NUMBER_MAX - ROOM_NUMBER_MIN)

UINT EmptyRoomList[TOT_ROOM_CNT];
PGAME_ROOM RoomList[TOT_ROOM_CNT] = { 0 };
UINT CurrentRoomNum;

VOID InitRoomManager(VOID)
{
    for (int i = 0; i < TOT_ROOM_CNT; i++) EmptyRoomList[i] = i;
    CurrentRoomNum = 0;
}

BOOL CreateRoom(
    _Inout_ PCONNECTION_INFO pConnInfo,
    _In_z_ const char* NickName,
    _In_opt_z_ const char* Password)
{
    PGAME_ROOM pRoom = NULL;
    BOOL bSuccess = TRUE;
    if (pConnInfo->pRoom)
    {
        return SendCreateRoom(pConnInfo, FALSE, 0, 0, "You are already in a room.");
    }
    if (strlen(NickName) > PLAYER_NICK_MAXLEN)
    {
        return SendCreateRoom(pConnInfo, FALSE, 0, 0, "Nick name too long.");
    }
    if (Password)
    {
        if (strlen(Password) > ROOM_PASSWORD_MAXLEN)
            return SendCreateRoom(pConnInfo, FALSE, 0, 0, "Password too long.");

        if (Password[0] == '\0')
            return SendCreateRoom(pConnInfo, FALSE, 0, 0, "Empty password field.");
    }

    AcquireSRWLockExclusive(&RoomPoolLock);
    __try
    {
        if (CurrentRoomNum == TOT_ROOM_CNT) // all room is full.
        {
            bSuccess = SendCreateRoom(pConnInfo, FALSE, 0, 0, "All room number is occupied, no room left.");
            __leave;
        }

        pRoom = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GAME_ROOM));
        if (!pRoom)
            __leave;

        // randomly choose one unused room number in EmptyRoomList
        // swap it with the last one in EmptyRoomList
        // and take that as room number
        UINT RandNum;
        if (rand_s(&RandNum) != 0)
            __leave;

        RandNum %= (TOT_ROOM_CNT - CurrentRoomNum);
        pRoom->RoomNumber = EmptyRoomList[RandNum];
        EmptyRoomList[RandNum] = EmptyRoomList[TOT_ROOM_CNT - CurrentRoomNum];
        CurrentRoomNum++;

        InterlockedIncrement64(&(pRoom->RefCnt));

        pConnInfo->pRoom = pRoom;
        pConnInfo->WaitingIndex = pRoom->WaitingCount++;

        PPLAYER_INFO pPlayerWaitingInfo = &pRoom->WaitingList[pConnInfo->WaitingIndex];
        pPlayerWaitingInfo->pConnInfo = pConnInfo;
        pPlayerWaitingInfo->GameID = pRoom->IDCount++;
        pPlayerWaitingInfo->bIsRoomOwner = TRUE;
        StringCbCopyA(pPlayerWaitingInfo->NickName, PLAYER_NICK_MAXLEN, NickName);
        StringCbCopyA(pPlayerWaitingInfo->Avatar, PLAYER_NICK_MAXLEN, "");

        if (Password)
            StringCbCopyA(pRoom->Password, ROOM_PASSWORD_MAXLEN, Password);

        InitializeSRWLock(&(pRoom->PlayerListLock));

        RoomList[pRoom->RoomNumber] = pRoom;

        Log(LOG_INFO, L"room %1!d! is opened.", pRoom->RoomNumber + ROOM_NUMBER_MIN);

        bSuccess = SendCreateRoom(pConnInfo, TRUE, pRoom->RoomNumber, 0, NULL);

        // TODO: I'm not sure what should I do if anything went wrong when broadcasting...
        // so I ignored the return value for now
        BroadcastRoomStatus(pRoom);
    }
    __finally
    {
        ReleaseSRWLockExclusive(&RoomPoolLock);
    }

    return bSuccess;
}

BOOL JoinRoom(
    _In_ UINT RoomNum,
    _Inout_ PCONNECTION_INFO pConnInfo,
    _In_z_ const char* NickName,
    _In_opt_z_ const char* Password)
{
    BOOL bSuccess = TRUE;
    if (pConnInfo->pRoom)
        return SendJoinRoom(pConnInfo, FALSE, 0, "You are already in a room.");

    if (strlen(NickName) > PLAYER_NICK_MAXLEN)
        return SendJoinRoom(pConnInfo, FALSE, 0, "Nick name too long.");

    AcquireSRWLockShared(&RoomPoolLock);
    __try
    {
        if (!RoomList[RoomNum])
        {
            SendJoinRoom(pConnInfo, FALSE, 0, "Room does not exist.");
            __leave;
        }

        PGAME_ROOM pRoom = RoomList[RoomNum];

        // check if password is correct, room is full, game is started, or NickName is duplicated.
        if (pRoom->Password[0] != '\0')
        {
            if (!Password)
            {
                SendJoinRoom(pConnInfo, FALSE, 0, "Password is required.");
                __leave;
            }
            if (strcmp(Password, pRoom->Password))
            {
                SendJoinRoom(pConnInfo, FALSE, 0, "Wrong password.");
                __leave;
            }
        }

        AcquireSRWLockExclusive(&pRoom->PlayerListLock);

        __try
        {
            if (pRoom->bGaming)
            {
                SendJoinRoom(pConnInfo, FALSE, 0, "The game has started already.");
                __leave;
            }

            if (pRoom->WaitingCount == ROOM_PLAYER_MAX)
            {
                SendJoinRoom(pConnInfo, FALSE, 0, "The room is full.");
                __leave;
            }

            for (UINT i = 0; i < pRoom->WaitingCount; i++)
            {
                if (strcmp(NickName, pRoom->WaitingList[i].NickName) == 0)
                {
                    SendJoinRoom(pConnInfo, FALSE, 0, "Duplicate nickname, try another.");
                    __leave;
                }
            }

            // pRoom is copied to pConnInfo from now on, add ref.
            InterlockedIncrement64(&(pRoom->RefCnt));
            pConnInfo->pRoom = pRoom;

            pConnInfo->WaitingIndex = pRoom->WaitingCount++;

            PPLAYER_INFO pPlayerWaitingInfo = &pRoom->WaitingList[pConnInfo->WaitingIndex];
            pPlayerWaitingInfo->pConnInfo = pConnInfo;
            pPlayerWaitingInfo->GameID = pRoom->IDCount++;
            pPlayerWaitingInfo->bIsRoomOwner = FALSE;
            StringCbCopyA(pPlayerWaitingInfo->NickName, PLAYER_NICK_MAXLEN, NickName);
            StringCbCopyA(pPlayerWaitingInfo->Avatar, PLAYER_NICK_MAXLEN, "");

            bSuccess &= SendJoinRoom(pConnInfo, TRUE, pPlayerWaitingInfo->GameID, NULL);

            // TODO: I'm not sure what should I do if anything went wrong when broadcasting...
            // so I ignored the return value for now
            BroadcastRoomStatus(pRoom);
        }
        __finally
        {
            ReleaseSRWLockExclusive(&pRoom->PlayerListLock);
        }
    }
    __finally
    {
        ReleaseSRWLockShared(&RoomPoolLock);
    }

    return bSuccess;
}

// Leave the room if the current user is inside one.
// And if there's no one in the room, it will be closed.
// Room owner will be transferred if the current user is room owner
// Will boardcast room status to the rest of player in room after leaving.
VOID LeaveRoom(_Inout_ PCONNECTION_INFO pConnInfo)
{
    AcquireSRWLockExclusive(&RoomPoolLock);
    __try
    {
        if (!pConnInfo->pRoom)
            __leave;

        LONG64 NewCnt = InterlockedDecrement64(&(pConnInfo->pRoom->RefCnt));
        if (NewCnt == 0)
        {
            Log(LOG_INFO, L"room %1!d! is closed.", pConnInfo->pRoom->RoomNumber + ROOM_NUMBER_MIN);

            EmptyRoomList[TOT_ROOM_CNT - CurrentRoomNum] = pConnInfo->pRoom->RoomNumber;
            CurrentRoomNum--;
            RoomList[pConnInfo->pRoom->RoomNumber] = NULL;
            HeapFree(GetProcessHeap(), 0, pConnInfo->pRoom);
        }
        else
        {
            PGAME_ROOM pRoom = pConnInfo->pRoom;
            AcquireSRWLockExclusive(&pRoom->PlayerListLock);

            for (UINT i = pConnInfo->WaitingIndex; i < pRoom->WaitingCount - 1; i++)
            {
                pRoom->WaitingList[i] = pRoom->WaitingList[i + 1];
                pRoom->WaitingList[i].pConnInfo->WaitingIndex = i;
            }
            pRoom->WaitingCount--;

            if (pConnInfo->WaitingIndex == 0) // transfer room owner if needed
            {
                pRoom->WaitingList[0].bIsRoomOwner = TRUE;
            }

            // Player is offline. set the corresponding field to NULL.
            pRoom->PlayingList[pConnInfo->PlayingIndex].pConnInfo = NULL;

            BroadcastRoomStatus(pRoom);
            ReleaseSRWLockExclusive(&pRoom->PlayerListLock);
        }
    }
    __finally
    {
        pConnInfo->pRoom = NULL;
        ReleaseSRWLockExclusive(&RoomPoolLock);
    }
}

BOOL ChangeAvatar(_Inout_ PCONNECTION_INFO pConnInfo, _In_z_ const char* Avatar)
{
    // TODO: add response for ChangeAvatar?
    if (strlen(Avatar) > PLAYER_AVATAR_MAXLEN)
    {
        return TRUE;
    }
    PGAME_ROOM pRoom = pConnInfo->pRoom;

    if (pRoom->bGaming)
    {
        return TRUE; // You can't change avatar when game started.
    }
    AcquireSRWLockShared(&pRoom->PlayerListLock);

    StringCbCopyA(pConnInfo->pRoom->WaitingList[pConnInfo->WaitingIndex].Avatar, PLAYER_AVATAR_MAXLEN, Avatar);

    // TODO: I'm not sure what should I do if anything went wrong when broadcasting...
    // so I ignored the return value for now
    BroadcastRoomStatus(pRoom);
    ReleaseSRWLockShared(&pRoom->PlayerListLock);
    return TRUE;
}

// assign a random role to RoleList based on PlayingCount
static BOOL AssignRole(_Inout_ PGAME_ROOM pRoom)
{
    UINT RoleList5[] =  { ROLE_MERLIN, ROLE_PERCIVAL, ROLE_LOYALIST,                                              ROLE_MORGANA,  ROLE_ASSASSIN };
    UINT RoleList6[] =  { ROLE_MERLIN, ROLE_PERCIVAL, ROLE_LOYALIST, ROLE_LOYALIST,                               ROLE_MORGANA,  ROLE_ASSASSIN };
    UINT RoleList7[] =  { ROLE_MERLIN, ROLE_PERCIVAL, ROLE_LOYALIST, ROLE_LOYALIST,                               ROLE_MORGANA,  ROLE_OBERON,   ROLE_ASSASSIN };
    UINT RoleList8[] =  { ROLE_MERLIN, ROLE_PERCIVAL, ROLE_LOYALIST, ROLE_LOYALIST, ROLE_LOYALIST,                ROLE_MORGANA,  ROLE_ASSASSIN, ROLE_MINIONS };
    UINT RoleList9[] =  { ROLE_MERLIN, ROLE_PERCIVAL, ROLE_LOYALIST, ROLE_LOYALIST, ROLE_LOYALIST, ROLE_LOYALIST, ROLE_MORDRED,  ROLE_MORGANA, ROLE_ASSASSIN };
    UINT RoleList10[] = { ROLE_MERLIN, ROLE_PERCIVAL, ROLE_LOYALIST, ROLE_LOYALIST, ROLE_LOYALIST, ROLE_LOYALIST, ROLE_MORDRED,  ROLE_MORGANA, ROLE_OBERON,  ROLE_ASSASSIN };

    UINT* List[] = { RoleList5, RoleList6, RoleList7, RoleList8, RoleList9, RoleList10 };

    // TODO: assert(pRoom->PlayingCount - ROOM_PLAYER_MIN < _countof(List))

    UINT* pList = List[pRoom->PlayingCount - ROOM_PLAYER_MIN];
    for (UINT i = 0; i < pRoom->PlayingCount; i++)
    {
        UINT RandNum;
        if (rand_s(&RandNum) != 0)
            return FALSE;

        RandNum %= (pRoom->PlayingCount - i);
        pRoom->RoleList[i] = pList[RandNum];
        pList[RandNum] = pList[pRoom->PlayingCount - i - 1];
    }
    return TRUE;
}

BOOL StartGame(_Inout_ PCONNECTION_INFO pConnInfo)
{
    PGAME_ROOM pRoom = pConnInfo->pRoom;
    if (!pRoom)
        return SendStartGame(pConnInfo, FALSE, "You are not in a room.");

    BOOL bSuccess = FALSE;
    AcquireSRWLockExclusive(&pConnInfo->pRoom->PlayerListLock);
    __try
    {
        if (!pRoom->WaitingList[pConnInfo->WaitingIndex].bIsRoomOwner)
        {
            bSuccess = SendStartGame(pConnInfo, FALSE, "You are not room owner.");
            __leave;
        }
        if (pRoom->WaitingCount < ROOM_PLAYER_MIN)
        {
            bSuccess = SendStartGame(pConnInfo, FALSE, "Too less player to start game.");
            __leave;
        }
        if (pRoom->bGaming)
        {
            bSuccess = SendStartGame(pConnInfo, FALSE, "Game already started.");
            __leave;
        }

        // copy WaitingList to PlayingList, update index as well.
        for (UINT i = 0; i < pRoom->WaitingCount; i++)
        {
            pRoom->PlayingList[i] = pRoom->WaitingList[i];
            pRoom->PlayingList[i].pConnInfo->PlayingIndex = i;
        }
        pRoom->PlayingCount = pRoom->WaitingCount;

        if (!AssignRole(pRoom))
        {
            Log(LOG_ERROR, L"AssignRole failed.");
            bSuccess = SendStartGame(pConnInfo, FALSE, "Server internal error. failed to assign role.");
            __leave;
        }

        // rand a leader, and set fairy if needed.
        UINT RandNum;
        if (rand_s(&RandNum) != 0)
            __leave;
        pRoom->LeaderIndex = RandNum % (pRoom->PlayingCount);

        if (pRoom->PlayingCount >= ENABLE_FAIRY_THRESHOLD)
        {
            pRoom->bFairyEnabled = TRUE;
            pRoom->FairyIndex = (pRoom->LeaderIndex + pRoom->PlayingCount - 1) % (pRoom->PlayingCount);
        }
        else
        {
            pRoom->bFairyEnabled = FALSE;
            pRoom->FairyIndex = 0;
        }
        pRoom->bGaming = TRUE;

        bSuccess = SendStartGame(pConnInfo, TRUE, NULL);

        for (UINT i = 0; i < pRoom->PlayingCount; i++)
        {
            UINT FairyID = pRoom->bFairyEnabled ? pRoom->PlayingList[pRoom->FairyIndex].GameID : 0;
            SendBeginGame(pRoom->PlayingList[i].pConnInfo, pRoom->RoleList[i], pRoom->bFairyEnabled, FairyID);
        }
    }
    __finally
    {
        ReleaseSRWLockExclusive(&pConnInfo->pRoom->PlayerListLock);
    }

    return bSuccess;
}

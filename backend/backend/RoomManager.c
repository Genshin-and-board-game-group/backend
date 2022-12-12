#define _CRT_RAND_S
#include <stdlib.h>
#include <strsafe.h>

#include "common.h"
#include "RoomManager.h"
#include "HttpSendRecv.h"
#include "MessageSender.h"
// This lock must be acquired when creating / deleting / entering / leaving a room



// ��Ϸÿ�����������
UINT  Team_Member_Cnt[11][6] = {
            {0},//0
            {0},//1
            {0},//2
            {0},//3
            {0},//4
            {2,3,2,3,3},//5
            {2,3,4,3,4},//6
            {2,3,4,4,4},//7
            {3,4,4,5,5},//8
            {3,4,4,5,5},//9
            {3,4,4,5,5} };//10

//���ٸ���Ʊ������Ϸʧ��
UINT  Task_Fail[11][6] = {
            {0},//0
            {0},//1
            {0},//2
            {0},//3
            {0},//4
            {1,1,1,1,1},//5
            {1,1,1,1,1},//6
            {1,1,1,2,1},//7
            {1,1,1,2,1},//8
            {1,1,1,2,1},//9
            {1,1,1,2,1} };//10


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

// return FALSE when not found in the room.
// Index is stored in pIndex
static BOOL GetGamingIndexByID(_In_ PGAME_ROOM pRoom, _In_ UINT ID, _Out_ UINT *pIndex)
{
    for (UINT i = 0; i < pRoom->PlayingCount; i++)
    {
        if (pRoom->PlayingList[i].GameID == ID)
        {
            *pIndex = i;
            return TRUE;
        }
    }
    *pIndex = 0;
    return FALSE;
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
        return ReplyCreateRoom(pConnInfo, FALSE, 0, 0, "You are already in a room.");
    }
    if (strlen(NickName) > PLAYER_NICK_MAXLEN)
    {
        return ReplyCreateRoom(pConnInfo, FALSE, 0, 0, "Nick name too long.");
    }
    if (Password)
    {
        if (strlen(Password) > ROOM_PASSWORD_MAXLEN)
            return ReplyCreateRoom(pConnInfo, FALSE, 0, 0, "Password too long.");

        if (Password[0] == '\0')
            return ReplyCreateRoom(pConnInfo, FALSE, 0, 0, "Empty password field.");
    }

    AcquireSRWLockExclusive(&RoomPoolLock);
    __try
    {
        if (CurrentRoomNum == TOT_ROOM_CNT) // all room is full.
        {
            bSuccess = ReplyCreateRoom(pConnInfo, FALSE, 0, 0, "All room number is occupied, no room left.");
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

        if (!ReplyCreateRoom(pConnInfo, TRUE, pRoom->RoomNumber, 0, NULL))
            __leave;

        if (!BroadcastRoomStatus(pRoom))
            __leave;
        bSuccess = TRUE;
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
        return ReplyJoinRoom(pConnInfo, FALSE, 0, "You are already in a room.");

    if (strlen(NickName) > PLAYER_NICK_MAXLEN)
        return ReplyJoinRoom(pConnInfo, FALSE, 0, "Nick name too long.");

    AcquireSRWLockShared(&RoomPoolLock);
    __try
    {
        if (!RoomList[RoomNum])
        {
            ReplyJoinRoom(pConnInfo, FALSE, 0, "Room does not exist.");
            __leave;
        }

        PGAME_ROOM pRoom = RoomList[RoomNum];

        // check if password is correct, room is full, game is started, or NickName is duplicated.
        if (pRoom->Password[0] != '\0')
        {
            if (!Password)
            {
                ReplyJoinRoom(pConnInfo, FALSE, 0, "Password is required.");
                __leave;
            }
            if (strcmp(Password, pRoom->Password))
            {
                ReplyJoinRoom(pConnInfo, FALSE, 0, "Wrong password.");
                __leave;
            }
        }

        AcquireSRWLockExclusive(&pRoom->PlayerListLock);

        __try
        {
            if (pRoom->bGaming)
            {
                ReplyJoinRoom(pConnInfo, FALSE, 0, "The game has started already.");
                __leave;
            }

            if (pRoom->WaitingCount == ROOM_PLAYER_MAX)
            {
                ReplyJoinRoom(pConnInfo, FALSE, 0, "The room is full.");
                __leave;
            }

            for (UINT i = 0; i < pRoom->WaitingCount; i++)
            {
                if (strcmp(NickName, pRoom->WaitingList[i].NickName) == 0)
                {
                    ReplyJoinRoom(pConnInfo, FALSE, 0, "Duplicate nickname, try another.");
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

            if (!ReplyJoinRoom(pConnInfo, TRUE, pPlayerWaitingInfo->GameID, NULL))
                __leave;

            if (!BroadcastRoomStatus(pRoom))
                __leave;

            bSuccess = TRUE;
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
        return TRUE;

    PGAME_ROOM pRoom = pConnInfo->pRoom;
    BOOL bSuccess = FALSE;

    AcquireSRWLockShared(&pRoom->PlayerListLock);
    __try
    {
        if (pRoom->bGaming) // You can't change avatar when game started.
            __leave;

        StringCbCopyA(pConnInfo->pRoom->WaitingList[pConnInfo->WaitingIndex].Avatar, PLAYER_AVATAR_MAXLEN, Avatar);
        bSuccess = BroadcastRoomStatus(pRoom);
    }
    __finally
    {
        ReleaseSRWLockShared(&pRoom->PlayerListLock);
    }
    return bSuccess;
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
        return ReplyStartGame(pConnInfo, FALSE, "You are not in a room.");

    BOOL bSuccess = FALSE;
    AcquireSRWLockExclusive(&pConnInfo->pRoom->PlayerListLock);
    __try
    {
        if (!pRoom->WaitingList[pConnInfo->WaitingIndex].bIsRoomOwner)
        {
            bSuccess = ReplyStartGame(pConnInfo, FALSE, "You are not room owner.");
            __leave;
        }
        if (pRoom->WaitingCount < ROOM_PLAYER_MIN)
        {
            bSuccess = ReplyStartGame(pConnInfo, FALSE, "Too less player to start game.");
            __leave;
        }
        if (pRoom->bGaming)
        {
            bSuccess = ReplyStartGame(pConnInfo, FALSE, "Game already started.");
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
            bSuccess = ReplyStartGame(pConnInfo, FALSE, "Server internal error. failed to assign role.");
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

        bSuccess = ReplyStartGame(pConnInfo, TRUE, NULL);

        for (UINT i = 0; i < pRoom->PlayingCount; i++)
        {
            UINT FairyID = pRoom->bFairyEnabled ? pRoom->PlayingList[pRoom->FairyIndex].GameID : 0;
            SendBeginGame(pRoom->PlayingList[i].pConnInfo, pRoom->RoleList[i], pRoom->bFairyEnabled, FairyID);
        }

        if (!BroadcastSetLeader(pRoom, pRoom->LeaderIndex))
        {
            __leave;
        }
        for (int i = 0; i < pRoom->PlayingCount; i++) {
            if (pRoom->RoleList[i]==ROLE_MERLIN) {
                HINTLIST HINT[4];
                UINT HINT_CNT = 0;
                for (int j = 0; j < pRoom->PlayingCount; j++) {
                    if (pRoom->RoleList[j]==ROLE_ASSASSIN || pRoom->RoleList[j]==ROLE_MORGANA || pRoom->RoleList[j]==ROLE_OBERON || pRoom->RoleList[j] == ROLE_MINIONS)
                    {
                        HINT[HINT_CNT++] = (HINTLIST){  pRoom -> PlayingList[j]. GameID, HINT_BAD};
                    } 
                }
                SendRoleHint( pRoom -> PlayingList[i].pConnInfo, HINT_CNT, HINT);
            }
        }
        for (int i = 0; i < pRoom->PlayingCount; i++) {
            if (pRoom->RoleList[i] == ROLE_PERCIVAL) {
                HINTLIST HINT[4];
                UINT HINT_CNT = 0;
                for (int j = 0; j < pRoom->PlayingCount; j++) {
                    if ( pRoom->RoleList[j] == ROLE_MORGANA || pRoom->RoleList[j] == ROLE_MERLIN)
                    {
                        HINT[HINT_CNT++] = (HINTLIST){ pRoom->PlayingList[j].GameID, HINT_MERLIN_OR_MORGANA };
                    }
                }
                SendRoleHint(pRoom->PlayingList[i].pConnInfo, HINT_CNT, HINT);
            }
        }
        for (int i = 0; i < pRoom->PlayingCount; i++) {
            if (pRoom->RoleList[i] == ROLE_ASSASSIN || pRoom->RoleList[i] == ROLE_MORGANA || pRoom->RoleList[i] == ROLE_MORDRED || pRoom->RoleList[i] == ROLE_MINIONS) {
                HINTLIST HINT[10];
                UINT HINT_CNT = 0;
                for (int j = 0; j < pRoom->PlayingCount; j++) 
                    if(i!=j){
                        if (pRoom->RoleList[j] == ROLE_MORGANA )
                        {
                            HINT[HINT_CNT++] = (HINTLIST){ pRoom->PlayingList[j].GameID, HINT_MORGANA };
                        }
                        if (pRoom->RoleList[j] == ROLE_MINIONS)
                        {
                            HINT[HINT_CNT++] = (HINTLIST){ pRoom->PlayingList[j].GameID, HINT_MINIONS };
                        }
                        if (pRoom->RoleList[j] == ROLE_MORDRED)
                        {
                            HINT[HINT_CNT++] = (HINTLIST){ pRoom->PlayingList[j].GameID, HINT_MORDRED };
                        }
                        if (pRoom->RoleList[j] == ROLE_ASSASSIN)
                        {
                            HINT[HINT_CNT++] = (HINTLIST){ pRoom->PlayingList[j].GameID, HINT_ASSASSIN };
                        }
                    }
                SendRoleHint(pRoom->PlayingList[i].pConnInfo, HINT_CNT, HINT);
            }
        }
    }
    __finally
    {
        ReleaseSRWLockExclusive(&pConnInfo->pRoom->PlayerListLock);
    }

    return bSuccess;
}

BOOL PlayerSelectTeam(_Inout_ PCONNECTION_INFO pConnInfo, _In_ UINT TeamMemberCnt, _In_ UINT32 TeamMemberList[])
{
    PGAME_ROOM pRoom = pConnInfo->pRoom;
    BOOL bSuccess = FALSE;
    if (!pRoom)
        return ReplyPlayerSelectTeam(pConnInfo, FALSE, "You are not in a room.");
    if (pRoom->Game_Win>=3)
        return ReplyPlayerSelectTeam(pConnInfo, FALSE, "Good guys has win 3 rouonds.");
    AcquireSRWLockExclusive(&pRoom->PlayerListLock);
    __try {

        if (!pRoom->bGaming)
        {
            bSuccess = ReplyPlayerSelectTeam(pConnInfo, FALSE, "Game hasn't started yet.");
            __leave;
        }
        // TODO: ��鵱ǰ�Ƿ��Ƕӳ�ѡ���Ա����Ϸ�׶�


        // check the leader
        if (pRoom->LeaderIndex != pConnInfo->PlayingIndex)
        {
            bSuccess = ReplyPlayerSelectTeam(pConnInfo, FALSE, "You are not the leader.");
            __leave;
        }
        // check the number of people
        // TODO: fix wrong count
        if ( TeamMemberCnt > Team_Member_Cnt[pRoom->PlayingCount][pRoom->Rounds])
        {
            bSuccess = ReplyPlayerSelectTeam(pConnInfo, FALSE, "The number of people selected is wrong.");
            __leave;
        }
        // TODO: ��� TeamMemberList �е� ID �Ƿ���Ч
        
        pRoom->TeamMemberCnt = TeamMemberCnt;
        for (int i = 0; i < TeamMemberCnt; i++)
        {
            UINT Index;
            if (!GetGamingIndexByID(pRoom, TeamMemberList[i], &Index))
            {
                bSuccess = ReplyPlayerSelectTeam(pConnInfo, FALSE, "Invalid ID.");
                __leave;
            }
            pRoom->TeamMemberid[i] = TeamMemberList[i];
        }

        if (!ReplyPlayerSelectTeam(pConnInfo, TRUE, NULL))
            __leave;

        if (!BroadcastSelectTeam(pRoom, pRoom->TeamMemberCnt, pRoom->TeamMemberid))
            __leave;

        bSuccess = TRUE;
    }
    __finally{
        ReleaseSRWLockExclusive(&pRoom->PlayerListLock);
    }
    return bSuccess;
}

BOOL PlayerConfirmTeam(_Inout_ PCONNECTION_INFO pConnInfo)
{
    PGAME_ROOM pRoom = pConnInfo->pRoom;
    BOOL bSuccess = FALSE;
    if (!pRoom)
        return ReplyPlayerConfirmTeam(pConnInfo, FALSE, "You are not in a room.");

    if (pRoom->Game_Win >= 3)
        return ReplyPlayerConfirmTeam(pConnInfo, FALSE, "Good guys has win 3 rouonds.");
    AcquireSRWLockExclusive(&pRoom->PlayerListLock);
    __try {
        // check bGaming
        if (!pRoom->bGaming)
        {
            bSuccess = ReplyPlayerConfirmTeam(pConnInfo, FALSE, "Game hasn't started yet.");
            __leave;
        }

        // TODO: ��鵱ǰ�Ƿ��Ƕӳ�ѡ���Ա����Ϸ�׶�

        // check the leader
        if (pRoom->LeaderIndex != pConnInfo->PlayingIndex)
        {
            bSuccess = ReplyPlayerConfirmTeam(pConnInfo, FALSE, "You are not the leader.");
            __leave;
        }

        // TODO: fix wrong count
        if (pRoom->TeamMemberCnt != Team_Member_Cnt[pRoom->PlayingCount][pRoom->Rounds])
        {
            bSuccess = ReplyPlayerConfirmTeam(pConnInfo, FALSE, "The number of people selected is wrong.");
            __leave;
        }

        if (!ReplyPlayerConfirmTeam(pConnInfo, TRUE, NULL))
            __leave;
        if (!BroadcastConfirmTeam(pRoom))
            __leave;
        bSuccess = TRUE;
    }
    __finally {
        ReleaseSRWLockExclusive(&pRoom->PlayerListLock);
    }
    return bSuccess;
}

BOOL PlayerVoteTeam(_Inout_ PCONNECTION_INFO pConnInfo, _In_ BOOL bVote)
{
    PGAME_ROOM pRoom = pConnInfo->pRoom;
    BOOL bSuccess = FALSE;
    if (!pRoom)
        return ReplyPlayerVoteTeam(pConnInfo, FALSE, "You are not in a room.");
    if (pRoom->Game_Win >= 3)
        return ReplyPlayerVoteTeam(pConnInfo, FALSE, "Good guys has win 3 rouonds.");
    AcquireSRWLockExclusive(&pRoom->PlayerListLock);
    __try {
        if (!pRoom->bGaming)
        {
            bSuccess = ReplyPlayerVoteTeam(pConnInfo, FALSE, "Game hasn't started yet.");
            __leave;
        }
        
        // TODO: ��鵱ǰ�Ƿ������ͶƱ�׶�
        for (int i = 0; i < pRoom->VotedCount; i++) {
            if (pRoom->VotedIDList[i].ID == pConnInfo->PlayingIndex) {
                bSuccess = ReplyPlayerVoteTeam(pConnInfo, FALSE, "You've already voted.");
                __leave;
            }
        }
        if (!ReplyPlayerVoteTeam(pConnInfo, TRUE, NULL))
            __leave;


        pRoom->VotedList[pRoom->VotedCount] = pConnInfo->PlayingIndex;
        pRoom->VotedIDList[pRoom->VotedCount++] = (VOTELIST){ pConnInfo->PlayingIndex ,bVote };
        
        if (!BroadcastVoteTeamProgress(pRoom, pRoom->VotedCount, pRoom->VotedList))
            __leave;
        if (pRoom->VotedCount == pRoom->PlayingCount) {
            UINT cnt = 0;
            for (int i = 0; i < pRoom->VotedCount; i++) {
                cnt += pRoom->VotedIDList[i].VoteResult;
            }
            if (!BroadcastVoteTeam(pRoom, cnt+cnt > pRoom->PlayingCount,pRoom->VotedCount, pRoom->VotedIDList))
                __leave;
            pRoom->VotedCount = 0;
            if (cnt + cnt <= pRoom->PlayingCount) 
            {
                pRoom->LeaderIndex = (pRoom->LeaderIndex + 1) % (pRoom->PlayingCount);
                if (!BroadcastSetLeader(pRoom, pRoom->LeaderIndex))
                {
                    __leave;
                }
                ++pRoom->Vote_Fail;
                if (pRoom->Vote_Fail == 5) {
                    if (!BroadcastEndGame(pRoom, FALSE, "VoteTeam Fail."))
                        __leave;
                    pRoom->bGaming = FALSE;
                    if (!BroadcastRoomStatus(pRoom))
                        __leave;
                }
            }
            else {
                pRoom->Vote_Fail=0;
            }
        }
        bSuccess = TRUE;
    }
    __finally {
        ReleaseSRWLockExclusive(&pRoom->PlayerListLock);
    }
    return bSuccess;
}

BOOL PlayerConductMission(_Inout_ PCONNECTION_INFO pConnInfo, _In_ BOOL bPerform)
{
    PGAME_ROOM pRoom = pConnInfo->pRoom;
    BOOL bSuccess = FALSE;
    if (!pRoom)
        return ReplyPlayerConductMission(pConnInfo, FALSE, "You are not in a room.");
    if (pRoom->Game_Win >= 3)
        return ReplyPlayerConductMission(pConnInfo, FALSE, "Good guys have win 3 rouonds.");
    AcquireSRWLockExclusive(&pRoom->PlayerListLock);
    __try
    {
        if (!pRoom->bGaming)
        {
            bSuccess = ReplyPlayerConductMission(pConnInfo, FALSE, "Game hasn't started yet.");
            __leave;
        }

        // TODO: ��鵱ǰ�Ƿ���ִ���������Ϸ�׶ε�...

        BOOL FLAG = 0;
        for (int i = 0; i < pRoom->TeamMemberCnt; i++)
        {
            FLAG |= pRoom->TeamMemberid[i] == pConnInfo->PlayingIndex;
        }

        if (!FLAG) {
            bSuccess = ReplyPlayerVoteTeam(pConnInfo, FALSE, "You are not in team.");
            __leave;
        }
        for (int i = 0; i < pRoom->DecidedCnt; i++) 
        {
            if (pRoom->DecidedIDList[i] == pConnInfo->PlayingIndex) {
                bSuccess = ReplyPlayerConductMission(pConnInfo, FALSE, "You've already voted.");
                __leave;
            }
        }
        pRoom->DecidedIDList[pRoom->DecidedCnt++] = pRoom->PlayingList[pConnInfo->PlayingIndex].GameID;
        if (!BroadcastMissionResultProgress(pRoom, pRoom->DecidedCnt, pRoom->DecidedIDList))
            __leave;
        // TODO :good or bad
        if (bPerform)++pRoom->Perform;
        else ++pRoom->Screw;
        // TODO: fix wrong count
        if (pRoom->DecidedCnt == Team_Member_Cnt[pRoom->PlayingCount][pRoom->Rounds]) {
            

            
            // TODO: fix wrong count

            if (!BroadcastMissionResult(pRoom, pRoom->Screw < Task_Fail[pRoom->PlayingCount][pRoom->Rounds], pRoom->Perform, pRoom->Screw))
                __leave;
            pRoom->LeaderIndex = (pRoom->LeaderIndex + 1) % (pRoom->PlayingCount);
            if (!BroadcastSetLeader(pRoom, pRoom->LeaderIndex))
            {
                __leave;
            }
            if (pRoom->Screw < Task_Fail[pRoom->PlayingCount][pRoom->Rounds])
                ++pRoom->Game_Win;
            pRoom->Rounds++;
            pRoom->DecidedCnt = pRoom->Screw = pRoom->Perform = 0;
            if (pRoom->Rounds - pRoom->Game_Win == 3) 
            {
                if (!BroadcastEndGame(pRoom, FALSE, "Bad Guys have win 3 Rounds."))
                    __leave;
            
                pRoom->bGaming = FALSE;
                if (!BroadcastRoomStatus(pRoom))
                    __leave;
            }
        }

        if (!ReplyPlayerConductMission(pConnInfo, TRUE, NULL))
            __leave;
        
        bSuccess = TRUE;
    }
    __finally
    {
        ReleaseSRWLockExclusive(&pConnInfo->pRoom->PlayerListLock);
    }
    return bSuccess;
}

BOOL PlayerFairyInspect(_Inout_ PCONNECTION_INFO pConnInfo, _In_ UINT ID)
{
    PGAME_ROOM pRoom = pConnInfo->pRoom;
    BOOL bSuccess = FALSE;
    if (!pRoom)
        return ReplyPlayerFairyInspect(pConnInfo, FALSE, "You are not in a room.");
    if (!pRoom->bFairyEnabled)
        return ReplyPlayerFairyInspect(pConnInfo, FALSE, "The room doesn't have the fairy.");
    AcquireSRWLockExclusive(&pRoom->PlayerListLock);
    __try
    {
        if (!pRoom->bGaming)
        {
            bSuccess = ReplyPlayerFairyInspect(pConnInfo, FALSE, "Game hasn't started yet.");
            __leave;
        }
        if (pConnInfo->PlayingIndex != pRoom->FairyIndex)
        {
            bSuccess = ReplyPlayerFairyInspect(pConnInfo, FALSE, "You are not fairy.");
            __leave;
        }

        UINT CheckIndex;
        if (!GetGamingIndexByID(pRoom, ID, &CheckIndex))
        {
            bSuccess = ReplyPlayerFairyInspect(pConnInfo, FALSE, "Invalid ID.");
            __leave;
        }

        // TODO: �ǵ���Ϸ���в���ѡ�Ѿ�������Ů��������Ĺ���

        if (pConnInfo->bBecomeFairy) {
            bSuccess = ReplyPlayerFairyInspect(pConnInfo, FALSE, "You had become fairy.");
            __leave;
        }

        pConnInfo->bBecomeFairy = 1;

        if (!ReplyPlayerFairyInspect(pConnInfo, TRUE, NULL))
            __leave;

        if(!BroadcastFairyInspect(pRoom,ID))
            __leave;

        // TODO: ������ݣ�pRoom->RoleList[CheckIndex]���û�
        // ͨ�� SendRoleHint ���� HINT_GOOD ���� HINT_BAD

        HINTLIST HintList[1];
        HintList[0].ID = ID;
        if (pRoom->RoleList[CheckIndex] == ROLE_MERLIN || pRoom->RoleList[CheckIndex] == ROLE_PERCIVAL || pRoom->RoleList[CheckIndex] == ROLE_LOYALIST) {
            
            HintList[0].HintType = HINT_GOOD;

            if (!SendRoleHint(pConnInfo, 1, HintList))
                __leave;
        }
        else {
            HintList[0].HintType = HINT_BAD;

            if (!SendRoleHint(pConnInfo, 1, HintList))
                __leave;
        }

        bSuccess = TRUE;
    }
    __finally
    {
        ReleaseSRWLockExclusive(&pConnInfo->pRoom->PlayerListLock);
    }
    return bSuccess;
}

BOOL PlayerAssassinate(_Inout_ PCONNECTION_INFO pConnInfo, _In_ UINT ID)
{
    PGAME_ROOM pRoom = pConnInfo->pRoom;
    BOOL bSuccess = FALSE;
    if (!pRoom)
        return ReplyPlayerAssassinate(pConnInfo, FALSE, "You are not in a room.");

    AcquireSRWLockExclusive(&pRoom->PlayerListLock);
    __try
    {
        if (!pRoom->bGaming)
        {
            bSuccess = ReplyPlayerAssassinate(pConnInfo, FALSE, "Game hasn't started yet.");
            __leave;
        }
        if (pRoom->RoleList[pConnInfo->PlayingIndex] != ROLE_ASSASSIN)
        {
            bSuccess = ReplyPlayerAssassinate(pConnInfo, FALSE, "You are not assassin.");
            __leave;
        }

        UINT AssassinateIndex;
        if (!GetGamingIndexByID(pRoom, ID, &AssassinateIndex))
        {
            bSuccess = ReplyPlayerAssassinate(pConnInfo, FALSE, "Invalid ID.");
            __leave;
        }

        if (!ReplyPlayerAssassinate(pConnInfo, TRUE, NULL))
            __leave;
        if (pRoom->RoleList[AssassinateIndex] == ROLE_MERLIN)
        {
            if (!BroadcastEndGame(pRoom, FALSE, "merlin was assassinated."))
                __leave;
        }
        else
        {
            if (!BroadcastEndGame(pRoom, TRUE, "assassin failed to kill merlin."))
                __leave;
        }
        pRoom->bGaming = FALSE;
        if (!BroadcastRoomStatus(pRoom))
            __leave;
        bSuccess = TRUE;
    }
    __finally
    {
        ReleaseSRWLockExclusive(&pConnInfo->pRoom->PlayerListLock);
    }
    return bSuccess;
}

BOOL PlayerTextMessage(_Inout_ PCONNECTION_INFO pConnInfo, _In_z_ const CHAR Message[])
{
    PGAME_ROOM pRoom = pConnInfo->pRoom;
    BOOL bSuccess = FALSE;
    if (!pRoom)
        return ReplyPlayerTextMessage(pConnInfo, FALSE, "You are not in a room.");

    AcquireSRWLockExclusive(&pRoom->PlayerListLock);
    __try
    {
        if (!pRoom->bGaming)
        {
            bSuccess = ReplyPlayerTextMessage(pConnInfo, FALSE, "Game hasn't started yet.");
            __leave;
        }
        if (!ReplyPlayerTextMessage(pConnInfo, TRUE, NULL))
            __leave;
        if (!BroadcastTextMessage(pRoom, pRoom->PlayingList[pConnInfo->PlayingIndex].GameID, Message))
            __leave;
        bSuccess = TRUE;
    }
    __finally
    {
        ReleaseSRWLockExclusive(&pConnInfo->pRoom->PlayerListLock);
    }
    return bSuccess;
}

#pragma once
#include "common.h"
#include "HttpSendRecv.h"

typedef struct
{
    UINT ID;
    UINT HintType;
}HINTLIST, *PHINTLIST;

typedef struct
{
    UINT ID;
    BOOL VoteResult;
}VOTELIST, *PVOTELIST;

BOOL SendCreateRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_ UINT RoomNum, _In_ UINT ID, _In_opt_z_ CHAR* Reason);

BOOL SendJoinRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_ UINT ID, _In_opt_z_ CHAR* Reason);

BOOL SendLeaveRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

BOOL SendStartGame(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

BOOL SendRoleHint(_In_ PCONNECTION_INFO pConnInfo, _In_ UINT HintCnt, _In_ HINTLIST HintList[]);

BOOL SendSetLeader(_In_ PCONNECTION_INFO pConnInfo, _In_ UINT ID);

BOOL BroadcastRoomStatus(_In_ PGAME_ROOM pRoom);

BOOL BroadcastBeginGame(_In_ PGAME_ROOM pRoom);

BOOL BroadcastSelectTeam(_In_ PGAME_ROOM pRoom, _In_ UINT TeamMemberCnt, _In_ UINT32 TeamMemberList[]);

BOOL BroadcastConfirmTeam(_In_ PGAME_ROOM pRoom);

BOOL BroadcastVoteTeamProgress(_In_ PGAME_ROOM pRoom, _In_ UINT VotedCnt, _In_ UINT32 VotedIDList[]);

BOOL BroadcastVoteTeam(_In_ PGAME_ROOM pRoom, _In_ UINT VoteListCnt, _In_ VOTELIST VoteList[]);

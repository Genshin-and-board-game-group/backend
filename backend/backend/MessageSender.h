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

BOOL SendBeginGame(_In_ PCONNECTION_INFO pConnInfo, _In_ UINT Role, _In_ BOOL bFairyEnabled, _In_ UINT FairyID);

BOOL SendRoleHint(_In_ PCONNECTION_INFO pConnInfo, _In_ UINT HintCnt, _In_ HINTLIST HintList[]);

BOOL SendSetLeader(_In_ PCONNECTION_INFO pConnInfo, _In_ UINT ID);

BOOL BroadcastRoomStatus(_In_ PGAME_ROOM pRoom);

BOOL BroadcastSelectTeam(_In_ PGAME_ROOM pRoom, _In_ UINT TeamMemberCnt, _In_ UINT32 TeamMemberList[]);

BOOL BroadcastConfirmTeam(_In_ PGAME_ROOM pRoom);

BOOL BroadcastVoteTeamProgress(_In_ PGAME_ROOM pRoom, _In_ UINT VotedCnt, _In_ UINT32 VotedIDList[]);

BOOL BroadcastVoteTeam(_In_ PGAME_ROOM pRoom, _In_ BOOL bVoteResult, _In_ UINT VoteListCnt, _In_ VOTELIST VoteList[]);

BOOL BroadcastMissionResultProgress(_In_ PGAME_ROOM pRoom, _In_ UINT DecidedCnt, _In_ UINT32 DecidedIDList[]);

BOOL BroadcastMissionResult(_In_ PGAME_ROOM pRoom, _In_ BOOL bMissionSuccess, _In_ UINT32 Perform, _In_ UINT32 Screw);

BOOL BroadcastFairyInspect(_In_ PGAME_ROOM pRoom, _In_ UINT InspectID);

BOOL BroadcastAssassinate(_In_ PGAME_ROOM pRoom, _In_ UINT AssassinateID);

BOOL BroadcastEndGame(_In_ PGAME_ROOM pRoom, _In_ BOOL bWin, _In_z_ CHAR Reason[]);

BOOL BroadcastTextMessage(_In_ PGAME_ROOM pRoom, _In_ UINT ID, _In_z_ CHAR Message[]);

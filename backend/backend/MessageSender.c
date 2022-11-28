#include "common.h"
#include "yyjson.h"
#include "JsonHandler.h"
#include "MessageSender.h"
#include "RoomManager.h"

static const CHAR* GetRoleString(UINT Role)
{
    // correspond with the ROLE_* MACRO
    // TODO: assert here
    static const CHAR* RoleTable[] = { NULL, "MERLIN", "PERCIVAL", "ASSASSIN", "MORDRED", "OBERON", "MORGANA", "LOYALIST", "MINIONS" };
    return RoleTable[Role];
}

static const CHAR* GetHintTypeString(UINT HintType)
{
    // correspond with the HINT_* MACRO
    // TODO: assert here
    static const CHAR* HintStrTable[] = { NULL, "GOOD", "BAD", "MERLIN_OR_MORGANA", "ASSASSIN", "MORDRED", "MORGANA", "MINIONS" };
    return HintStrTable[HintType];
}

static BOOL ReplySimpleMessage(_In_ PCONNECTION_INFO pConnInfo, _In_z_ CHAR szType[], _In_ BOOL bResult, _In_opt_z_ CHAR Reason[])
{
    // Create a mutable doc
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;
        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", szType);
        yyjson_mut_obj_add_str(doc, root, "result", bResult ? "success" : "fail");

        if (!bResult)
            yyjson_mut_obj_add_str(doc, root, "reason", Reason);

        bSuccess = SendJsonMessage(pConnInfo, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

// Only sends to player online & gaming
BOOL BroadcastGamingJsonMessage(_In_ PGAME_ROOM pRoom, _In_ yyjson_mut_doc* JsonDoc)
{
    for (UINT i = 0; i < pRoom->WaitingCount; i++) // this is also currently online user
    {
        if (!SendJsonMessage(pRoom->WaitingList[i].pConnInfo, JsonDoc))
            return FALSE;
    }
    return TRUE;
}

BOOL ReplyCreateRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_ UINT RoomNum, _In_ UINT ID, _In_opt_z_ CHAR* Reason)
{
    char szRoomNumber[10 + 1] = { 0 }; // MAXUINT32 tooks 10 char to store under decimal, without trailing zero.

    // Create a mutable doc
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;
        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "createRoom");
        yyjson_mut_obj_add_str(doc, root, "result", bResult ? "success" : "fail");

        if (bResult)
        {
            sprintf_s(szRoomNumber, _countof(szRoomNumber), "%d", RoomNum + ROOM_NUMBER_MIN);
            yyjson_mut_obj_add_str(doc, root, "roomNumber", szRoomNumber);
            yyjson_mut_obj_add_uint(doc, root, "ID", ID);
        }
        else
        {
            yyjson_mut_obj_add_str(doc, root, "reason", Reason);
        }

        bSuccess = SendJsonMessage(pConnInfo, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

BOOL ReplyJoinRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_ UINT ID, _In_opt_z_ CHAR* Reason)
{
    // Create a mutable doc
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;
        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "joinRoom");
        yyjson_mut_obj_add_str(doc, root, "result", bResult ? "success" : "fail");

        if (bResult)
        {
            yyjson_mut_obj_add_uint(doc, root, "ID", ID);
        }
        else
        {
            yyjson_mut_obj_add_str(doc, root, "reason", Reason);
        }

        bSuccess = SendJsonMessage(pConnInfo, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

BOOL ReplyLeaveRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason)
{
    return ReplySimpleMessage(pConnInfo, "leaveRoom", bResult, Reason);
}

BOOL ReplyStartGame(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason)
{
    return ReplySimpleMessage(pConnInfo, "startGame", bResult, Reason);
}

BOOL ReplyPlayerSelectTeam(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason)
{
    return ReplySimpleMessage(pConnInfo, "playerSelectTeam", bResult, Reason);
}

BOOL ReplyPlayerConfirmTeam(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason)
{
    return ReplySimpleMessage(pConnInfo, "playerConfirmTeam", bResult, Reason);
}

BOOL ReplyPlayerVoteTeam(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason)
{
    return ReplySimpleMessage(pConnInfo, "playerVoteTeam", bResult, Reason);
}

BOOL ReplyPlayerConductMission(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason)
{
    return ReplySimpleMessage(pConnInfo, "playerConductMission", bResult, Reason);
}

BOOL ReplyPlayerFairyInspect(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason)
{
    return ReplySimpleMessage(pConnInfo, "playerFairyInspect", bResult, Reason);
}

BOOL ReplyPlayerAssassinate(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason)
{
    return ReplySimpleMessage(pConnInfo, "playerAssassinate", bResult, Reason);
}

BOOL ReplyPlayerTextMessage(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason)
{
    return ReplySimpleMessage(pConnInfo, "playerTextMessage", bResult, Reason);
}

BOOL SendBeginGame(_In_ PCONNECTION_INFO pConnInfo, _In_ UINT Role, _In_ BOOL bFairyEnabled, _In_ UINT FairyID)
{
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;

        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "beginGame");
        yyjson_mut_obj_add_str(doc, root, "role", GetRoleString(Role));
        if (bFairyEnabled)
        {
            yyjson_mut_obj_add_uint(doc, root, "fairyID", FairyID);
        }
        bSuccess = SendJsonMessage(pConnInfo, doc); 
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

BOOL SendRoleHint(_In_ PCONNECTION_INFO pConnInfo, _In_ UINT HintCnt, _In_ HINTLIST HintList[])
{
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;

        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "roleHint");

        yyjson_mut_val* HintListVal = yyjson_mut_arr(doc);
        if (!HintListVal)
            __leave;

        for (UINT i = 0; i < HintCnt; i++)
        {
            yyjson_mut_val* HintVal = yyjson_mut_arr_add_obj(doc, HintListVal);
            yyjson_mut_obj_add_uint(doc, HintVal, "ID", HintList[i].ID);
            yyjson_mut_obj_add_str(doc, HintVal, "HintType", GetHintTypeString(HintList[i].HintType));
        }
        yyjson_mut_obj_add_val(doc, root, "HintList", HintListVal);

        bSuccess = SendJsonMessage(pConnInfo, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

BOOL BroadcastRoomStatus(_In_ PGAME_ROOM pRoom)
{
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;
        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "roomStatus");

        yyjson_mut_val* PlayerListVal = yyjson_mut_arr(doc);
        if (!PlayerListVal)
            __leave;

        if (pRoom->bGaming)
        {
            for (UINT i = 0; i < pRoom->PlayingCount; i++)
            {
                yyjson_mut_val* Player = yyjson_mut_arr_add_obj(doc, PlayerListVal);
                yyjson_mut_obj_add_str(doc, Player, "name", pRoom->PlayingList[i].NickName);
                yyjson_mut_obj_add_uint(doc, Player, "ID", pRoom->PlayingList[i].GameID);
                yyjson_mut_obj_add_str(doc, Player, "avatar", pRoom->PlayingList[i].Avatar);
                yyjson_mut_obj_add_bool(doc, Player, "isOwner", pRoom->PlayingList[i].bIsRoomOwner);
                yyjson_mut_obj_add_bool(doc, Player, "online", pRoom->PlayingList[i].pConnInfo != NULL);
            }
        }
        else
        {
            for (UINT i = 0; i < pRoom->WaitingCount; i++)
            {
                yyjson_mut_val* Player = yyjson_mut_arr_add_obj(doc, PlayerListVal);
                yyjson_mut_obj_add_str(doc, Player, "name", pRoom->WaitingList[i].NickName);
                yyjson_mut_obj_add_uint(doc, Player, "ID", pRoom->WaitingList[i].GameID);
                yyjson_mut_obj_add_str(doc, Player, "avatar", pRoom->WaitingList[i].Avatar);
                yyjson_mut_obj_add_bool(doc, Player, "isOwner", pRoom->WaitingList[i].bIsRoomOwner);
                yyjson_mut_obj_add_bool(doc, Player, "online", TRUE);
            }
        }
        yyjson_mut_obj_add_val(doc, root, "playerList", PlayerListVal);

        for (UINT i = 0; i < pRoom->WaitingCount; i++) // this is also currently online user
        {
            if (!SendJsonMessage(pRoom->WaitingList[i].pConnInfo, doc))
                __leave;
        }
        bSuccess = TRUE;
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

BOOL BroadcastSetLeader(_In_ PGAME_ROOM pRoom, _In_ UINT ID)
{
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;

        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "setLeader");
        yyjson_mut_obj_add_uint(doc, root, "ID", ID);

        bSuccess = BroadcastGamingJsonMessage(pRoom, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

BOOL BroadcastSelectTeam(_In_ PGAME_ROOM pRoom, _In_ UINT TeamSize, _In_ UINT32 TeamArr[])
{
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;

        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "selectTeam");
        yyjson_mut_val* TeamVal = yyjson_mut_arr_with_uint32(doc, TeamArr, TeamSize);
        if (!TeamVal)
            __leave;
        yyjson_mut_obj_add_val(doc, root, "team", TeamVal);

        bSuccess = BroadcastGamingJsonMessage(pRoom, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

BOOL BroadcastConfirmTeam(_In_ PGAME_ROOM pRoom)
{
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;

        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "confirmTeam");

        bSuccess = BroadcastGamingJsonMessage(pRoom, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

BOOL BroadcastVoteTeamProgress(_In_ PGAME_ROOM pRoom, _In_ UINT VotedCnt, _In_ UINT32 VotedIDList[])
{
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;

        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "voteTeamProgress");
        yyjson_mut_val* VotedVal = yyjson_mut_arr_with_uint32(doc, VotedIDList, VotedCnt);
        if (!VotedVal)
            __leave;
        yyjson_mut_obj_add_val(doc, root, "voted", VotedVal);

        bSuccess = BroadcastGamingJsonMessage(pRoom, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

BOOL BroadcastVoteTeam(_In_ PGAME_ROOM pRoom, _In_ BOOL bVoteResult, _In_ UINT VoteListCnt, _In_ VOTELIST VoteList[])
{
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;

        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "voteTeam");
        yyjson_mut_obj_add_bool(doc, root, "voteResult", bVoteResult);

        yyjson_mut_val* VoteListVal = yyjson_mut_arr(doc);
        if (!VoteListVal)
            __leave;

        for (UINT i = 0; i < VoteListCnt; i++)
        {
            yyjson_mut_val* VoteVal = yyjson_mut_arr_add_obj(doc, VoteListVal);
            yyjson_mut_obj_add_uint(doc, VoteVal, "ID", VoteList[i].ID);
            yyjson_mut_obj_add_bool(doc, VoteVal, "vote", VoteList[i].VoteResult);
        }
        yyjson_mut_obj_add_val(doc, root, "voteList", VoteListVal);

        bSuccess = BroadcastGamingJsonMessage(pRoom, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

BOOL BroadcastMissionResultProgress(_In_ PGAME_ROOM pRoom, _In_ UINT DecidedCnt, _In_ UINT32 DecidedIDList[])
{
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;

        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "missionResultProgress");
        yyjson_mut_val* DecidedVal = yyjson_mut_arr_with_uint32(doc, DecidedIDList, DecidedCnt);
        if (!DecidedVal)
            __leave;
        yyjson_mut_obj_add_val(doc, root, "decided", DecidedVal);

        bSuccess = BroadcastGamingJsonMessage(pRoom, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

BOOL BroadcastMissionResult(_In_ PGAME_ROOM pRoom, _In_ BOOL bMissionSuccess, _In_ UINT32 Perform, _In_ UINT32 Screw)
{
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;

        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "missionResult");
        yyjson_mut_obj_add_bool(doc, root, "missionSuccess", bMissionSuccess);
        yyjson_mut_obj_add_uint(doc, root, "perform", Perform);
        yyjson_mut_obj_add_uint(doc, root, "screw", Screw);

        bSuccess = BroadcastGamingJsonMessage(pRoom, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

BOOL BroadcastFairyInspect(_In_ PGAME_ROOM pRoom, _In_ UINT InspectID)
{
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;

        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "fairyInspect");
        yyjson_mut_obj_add_uint(doc, root, "ID", InspectID);

        bSuccess = BroadcastGamingJsonMessage(pRoom, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

BOOL BroadcastEndGame(_In_ PGAME_ROOM pRoom, _In_ BOOL bWin, _In_z_ CHAR Reason[])
{
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;

        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "endGame");
        yyjson_mut_obj_add_bool(doc, root, "win", bWin);
        yyjson_mut_obj_add_str(doc, root, "reason", Reason);

        yyjson_mut_val* RoleListVal = yyjson_mut_arr(doc);
        if (!RoleListVal)
            __leave;

        for (UINT i = 0; i < pRoom->PlayingCount; i++)
        {
            yyjson_mut_val* RoleVal = yyjson_mut_arr_add_obj(doc, RoleListVal);
            yyjson_mut_obj_add_uint(doc, RoleVal, "ID", pRoom->PlayingList[i].GameID);
            yyjson_mut_obj_add_str(doc, RoleVal, "role", GetRoleString(pRoom->RoleList[i]));
        }
        yyjson_mut_obj_add_val(doc, root, "roleList", RoleListVal);

        bSuccess = BroadcastGamingJsonMessage(pRoom, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

BOOL BroadcastTextMessage(_In_ PGAME_ROOM pRoom, _In_ UINT ID, _In_z_ CHAR Message[])
{
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;

        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "textMessage");
        yyjson_mut_obj_add_uint(doc, root, "ID", ID);
        yyjson_mut_obj_add_str(doc, root, "message", Message);

        bSuccess = BroadcastGamingJsonMessage(pRoom, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

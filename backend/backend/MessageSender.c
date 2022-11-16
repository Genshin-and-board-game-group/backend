#include "common.h"
#include "yyjson.h"
#include "JsonHandler.h"
#include "MessageSender.h"

static const CHAR* GetRoleString(UINT Role)
{
    // correspond with the ROLE_* MACRO
    // TODO: assert here
    static const CHAR* RoleTable[] = { NULL, "ROLE_MERLIN", "PERCIVAL", "ASSASSIN", "MORDRED", "OBERON", "MORGANA", "LOYALIST", "MINIONS" };
    return RoleTable[Role];
}

static const CHAR* GetHintTypeString(UINT HintType)
{
    // correspond with the HINT_* MACRO
    // TODO: assert here
    static const CHAR* HintStrTable[] = { NULL, "GOOD", "BAD", "MERLIN_OR_MORGANA", "ASSASSIN", "MORDRED", "MORGANA", "MINIONS" };
    return HintStrTable[HintType];
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

BOOL SendCreateRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_ UINT RoomNum, _In_ UINT ID, _In_opt_z_ CHAR* Reason)
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

BOOL SendJoinRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_ UINT ID, _In_opt_z_ CHAR* Reason)
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

BOOL SendLeaveRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason)
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
        yyjson_mut_obj_add_str(doc, root, "type", "leaveRoom");
        yyjson_mut_obj_add_str(doc, root, "result", bResult ? "success" : "fail");

        if (!bResult)
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

BOOL SendStartGame(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason)
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
        yyjson_mut_obj_add_str(doc, root, "type", "startGame");
        yyjson_mut_obj_add_str(doc, root, "result", bResult ? "success" : "fail");

        if (!bResult)
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
            yyjson_mut_val* Hint = yyjson_mut_arr_add_obj(doc, HintListVal);
            yyjson_mut_obj_add_uint(doc, Hint, "ID", HintList[i].ID);
            yyjson_mut_obj_add_str(doc, Hint, "HintType", GetHintTypeString(HintList[i].HintType));
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

BOOL SendSetLeader(_In_ PCONNECTION_INFO pConnInfo, _In_ UINT ID)
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
            SendJsonMessage(pRoom->WaitingList[i].pConnInfo, doc);
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

BOOL BroadcastBeginGame(_In_ PGAME_ROOM pRoom)
{
    // TODO: this function is actually not broadcasting...
    // it sends different message to different player.
    if (!pRoom->bGaming)
        return FALSE;

    for (UINT i = 0; i < pRoom->PlayingCount; i++)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        if (!doc)
            return FALSE;

        __try
        {
            yyjson_mut_val* root = yyjson_mut_obj(doc);
            if (!root)
                __leave;
            yyjson_mut_doc_set_root(doc, root);
            yyjson_mut_obj_add_str(doc, root, "type", "beginGame");
            yyjson_mut_obj_add_str(doc, root, "role", GetRoleString(pRoom->RoleList[i]));

            if (pRoom->FairyIndex != -1) // Send fairyID if enabled
            {
                yyjson_mut_obj_add_uint(doc, root, "fairyID", pRoom->PlayingList[pRoom->FairyIndex].GameID);
            }

            SendJsonMessage(pRoom->WaitingList[i].pConnInfo, doc);
        }
        __finally
        {
            yyjson_mut_doc_free(doc);
        }
    }
    return TRUE;
}

BOOL BroadcastSelectTeam(_In_ PGAME_ROOM pRoom, _In_ UINT TeamMemberCnt, _In_ UINT32 TeamMemberList[])
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
        yyjson_mut_val* TeamVal = yyjson_mut_arr_with_uint32(doc, TeamMemberList, TeamMemberCnt);
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

BOOL BroadcastVoteTeam(_In_ PGAME_ROOM pRoom, _In_ UINT VoteListCnt, _In_ VOTELIST VoteList[])
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

        yyjson_mut_val* VoteListVal = yyjson_mut_arr(doc);
        if (!VoteListVal)
            __leave;

        for (UINT i = 0; i < VoteListCnt; i++)
        {
            yyjson_mut_val* Hint = yyjson_mut_arr_add_obj(doc, VoteListVal);
            yyjson_mut_obj_add_uint(doc, Hint, "ID", VoteList[i].ID);
            yyjson_mut_obj_add_bool(doc, Hint, "vote", VoteList[i].VoteResult);
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

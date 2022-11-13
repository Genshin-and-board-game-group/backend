#include "common.h"
#include "yyjson.h"
#include "JsonHandler.h"

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

        yyjson_mut_val* PlayerList = yyjson_mut_arr(doc);

        if (pRoom->bGaming)
        {
            for (UINT i = 0; i < pRoom->PlayingCount; i++)
            {
                yyjson_mut_val* Player = yyjson_mut_arr_add_obj(doc, PlayerList);
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
                yyjson_mut_val* Player = yyjson_mut_arr_add_obj(doc, PlayerList);
                yyjson_mut_obj_add_str(doc, Player, "name", pRoom->WaitingList[i].NickName);
                yyjson_mut_obj_add_uint(doc, Player, "ID", pRoom->WaitingList[i].GameID);
                yyjson_mut_obj_add_str(doc, Player, "avatar", pRoom->WaitingList[i].Avatar);
                yyjson_mut_obj_add_bool(doc, Player, "isOwner", pRoom->WaitingList[i].bIsRoomOwner);
                yyjson_mut_obj_add_bool(doc, Player, "online", TRUE);
            }
        }

        yyjson_mut_obj_add_val(doc, root, "playerList", PlayerList);

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

static const CHAR* GetRoleStringByRole(UINT Role)
{
    // correspond with the ROLE_* MACRO
    static const CHAR* RoleTable[] = { NULL, "ROLE_MERLIN", "PERCIVAL", "ASSASSIN", "MORDRED", "OBERON", "MORGANA", "LOYALIST", "MINIONS" };
    return RoleTable[Role];
}

BOOL BroadcastBeginGame(_In_ PGAME_ROOM pRoom)
{
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
            yyjson_mut_obj_add_str(doc, root, "role", GetRoleStringByRole(pRoom->RoleList[i]));

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

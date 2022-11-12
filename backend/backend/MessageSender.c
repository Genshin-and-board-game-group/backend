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
            yyjson_mut_obj_add_sint(doc, root, "ID", ID);
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
            yyjson_mut_obj_add_sint(doc, root, "ID", ID);
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
            // TODO: unimplemented
            FIXME("unimplemented");
            DebugBreak();
        }
        else
        {
            for (UINT i = 0; i < pRoom->WaitingCount; i++)
            {
                yyjson_mut_val* Player = yyjson_mut_arr_add_obj(doc, PlayerList);
                yyjson_mut_obj_add_str(doc, Player, "name", pRoom->WaitingList[i].NickName);
                yyjson_mut_obj_add_sint(doc, Player, "ID", pRoom->WaitingList[i].GameID);
                yyjson_mut_obj_add_str(doc, Player, "avatar", pRoom->WaitingList[i].Avatar);
                yyjson_mut_obj_add_bool(doc, Player, "isOwner", pRoom->WaitingList[i].bIsRoomOwner);
                yyjson_mut_obj_add_bool(doc, Player, "online", TRUE);
            }
        }

        yyjson_mut_obj_add_val(doc, root, "playerList", PlayerList);

        for (UINT i = 0; i < pRoom->WaitingCount; i++) // this is also currently online user
        {
            bSuccess = SendJsonMessage(pRoom->WaitingList[i].pConnInfo, doc);
        }
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}

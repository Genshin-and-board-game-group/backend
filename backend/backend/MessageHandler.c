#include "common.h"
#include "HttpSendRecv.h"
#include "yyjson.h"
#include "MessageSender.h"
#include "RoomManager.h"
#include "MessageHandler.h"

BOOL HandleCreateRoom(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val *pJsonRoot)
{
    yyjson_val* pName = yyjson_obj_get(pJsonRoot, "name");
    if (!pName)
        return FALSE;
    const char* pNameStr = yyjson_get_str(pName);
    if (!pNameStr)
        return FALSE;

    yyjson_val* pPassword = yyjson_obj_get(pJsonRoot, "password");
    const char* pPasswordStr = NULL;
    if (pPassword)
        pPasswordStr = yyjson_get_str(pPassword);

    return CreateRoom(pConnInfo, pNameStr, pPasswordStr);
}

BOOL HandleJoinRoom(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    yyjson_val* pName = yyjson_obj_get(pJsonRoot, "name");
    if (!pName)
        return FALSE;
    const char* pNameStr = yyjson_get_str(pName);
    if (!pNameStr)
        return FALSE;

    yyjson_val* pPassword = yyjson_obj_get(pJsonRoot, "password");
    const char* pPasswordStr = NULL;
    if (pPassword)
        pPasswordStr = yyjson_get_str(pPassword);

    yyjson_val* pRoomNumber = yyjson_obj_get(pJsonRoot, "roomNumber");
    if (!pRoomNumber)
        return FALSE;
    const char* pRoomNumberStr = yyjson_get_str(pRoomNumber);
    if (!pRoomNumberStr)
        return FALSE;

    UINT RoomNumber = 0;
    for (UINT i = 0; pRoomNumberStr[i]; i++)
    {
        if (!isdigit(pRoomNumberStr[i]))
        {
            ReplyJoinRoom(pConnInfo, FALSE, 0, "incorrect room number");
            return TRUE;
        }
        RoomNumber *= 10;
        RoomNumber += pRoomNumberStr[i] - '0';
        if (RoomNumber > ROOM_NUMBER_MAX)
        {
            ReplyJoinRoom(pConnInfo, FALSE, 0, "incorrect room number");
            return TRUE;
        }
    }
    if (RoomNumber < ROOM_NUMBER_MIN)
    {
        ReplyJoinRoom(pConnInfo, FALSE, 0, "incorrect room number");
        return TRUE;
    }

    return JoinRoom(RoomNumber - ROOM_NUMBER_MIN, pConnInfo, pNameStr, pPasswordStr);
}

BOOL HandleChangeAvatar(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    yyjson_val* pAvatar = yyjson_obj_get(pJsonRoot, "avatar");
    if (!pAvatar)
        return FALSE;
    const char* pAvatarStr = yyjson_get_str(pAvatar);
    if (!pAvatarStr)
        return FALSE;

    return ChangeAvatar(pConnInfo, pAvatarStr);
}

BOOL HandleLeaveRoom(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    if (!pConnInfo->pRoom)
    {
        return ReplyLeaveRoom(pConnInfo, FALSE, "You are not in a room.");
    }

    LeaveRoom(pConnInfo);
    return ReplyLeaveRoom(pConnInfo, TRUE, NULL);
}

BOOL HandleStartGame(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    return StartGame(pConnInfo);
}

BOOL HandlePlayerSelectTeam(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    return TRUE;
}

BOOL HandlePlayerConfirmTeam(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    return TRUE;
}

BOOL HandlePlayerVoteTeam(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    return TRUE;
}

BOOL HandlePlayerConductMission(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    return TRUE;
}

BOOL HandlePlayerFairyInspect(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    return TRUE;
}

BOOL HandlePlayerAssassinate(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    yyjson_val *pAssassinateID = yyjson_obj_get(pJsonRoot, "ID");
    if (!pAssassinateID)
        return FALSE;

    if (!yyjson_is_uint(pAssassinateID))
        return FALSE;

    UINT AssassinateID = yyjson_get_uint(pAssassinateID);

    return PlayerAssassinate(pConnInfo, AssassinateID);
}

BOOL HandlePlayerTextMessage(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    return TRUE;
}

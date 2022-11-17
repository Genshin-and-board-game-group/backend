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
    yyjson_val *pTeam = yyjson_obj_get(pJsonRoot, "team");
    if (!yyjson_is_arr(pTeam))
        return FALSE;

    UINT TeamArr[ROOM_PLAYER_MAX] = { 0 };

    yyjson_val* val;
    yyjson_arr_iter iter;
    SIZE_T Size = unsafe_yyjson_get_len(pTeam);
    if (Size > ROOM_PLAYER_MAX)
        return FALSE;
    yyjson_arr_iter_init(pTeam, &iter);
    while ((val = yyjson_arr_iter_next(&iter))) {
        if (!yyjson_is_uint(val))
            return FALSE;
        TeamArr[iter.idx] = (UINT)yyjson_get_uint(val);
    }

    return PlayerSelectTeam(pConnInfo, (UINT)Size, TeamArr);
}

BOOL HandlePlayerConfirmTeam(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    return PlayerConfirmTeam(pConnInfo);
}

BOOL HandlePlayerVoteTeam(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    yyjson_val* pVote = yyjson_obj_get(pJsonRoot, "vote");
    if (!yyjson_is_bool(pVote))
        return FALSE;

    BOOL bVote = unsafe_yyjson_get_bool(pVote);
    return PlayerVoteTeam(pConnInfo, bVote);
}

BOOL HandlePlayerConductMission(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    yyjson_val* pPerform = yyjson_obj_get(pJsonRoot, "perform");
    if (!yyjson_is_bool(pPerform))
        return FALSE;

    BOOL bPerfrom = unsafe_yyjson_get_bool(pPerform);
    return PlayerConductMission(pConnInfo, bPerfrom);
}

BOOL HandlePlayerFairyInspect(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    yyjson_val* pID = yyjson_obj_get(pJsonRoot, "ID");
    if (!yyjson_is_uint(pID))
        return FALSE;

    UINT ID = (UINT)unsafe_yyjson_get_uint(pID);
    return PlayerFairyInspect(pConnInfo, ID);
}

BOOL HandlePlayerAssassinate(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    yyjson_val *pID = yyjson_obj_get(pJsonRoot, "ID");
    if (!yyjson_is_uint(pID))
        return FALSE;

    UINT ID = (UINT)yyjson_get_uint(pID);
    return PlayerAssassinate(pConnInfo, ID);
}

BOOL HandlePlayerTextMessage(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot)
{
    yyjson_val* pMessage = yyjson_obj_get(pJsonRoot, "message");
    if (!yyjson_is_str(pMessage))
        return FALSE;

    return PlayerTextMessage(pConnInfo, yyjson_get_str(pMessage));
}

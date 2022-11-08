#include "common.h"
#include "HttpSendRecv.h"
#include "yyjson.h"
#include "MessageSender.h"
#include "RoomManager.h"

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
            SendJoinRoom(pConnInfo, FALSE, 0, "incorrect room number");
            return TRUE;
        }
        RoomNumber *= 10;
        RoomNumber += pRoomNumberStr[i] - '0';
        if (RoomNumber > ROOM_NUMBER_MAX)
        {
            SendJoinRoom(pConnInfo, FALSE, 0, "incorrect room number");
            return TRUE;
        }
    }
    if (RoomNumber < ROOM_NUMBER_MIN)
    {
        SendJoinRoom(pConnInfo, FALSE, 0, "incorrect room number");
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

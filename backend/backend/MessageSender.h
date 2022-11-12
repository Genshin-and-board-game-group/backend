#pragma once
#include "common.h"
#include "HttpSendRecv.h"

BOOL SendCreateRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_ UINT RoomNum, _In_ UINT ID, _In_opt_z_ CHAR* Reason);

BOOL SendJoinRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_ UINT ID, _In_opt_z_ CHAR* Reason);

BOOL SendLeaveRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

BOOL BroadcastRoomStatus(_In_ PGAME_ROOM pRoom);

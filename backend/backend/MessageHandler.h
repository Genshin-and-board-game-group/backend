#pragma once
#include "common.h"
#include "HttpSendRecv.h"
#include "yyjson.h"

BOOL HandleCreateRoom(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

BOOL HandleJoinRoom(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

BOOL HandleChangeAvatar(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

BOOL HandleLeaveRoom(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

BOOL HandleStartGame(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

BOOL HandlePlayerSelectTeam(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

BOOL HandlePlayerConfirmTeam(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

BOOL HandlePlayerVoteTeam(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

BOOL HandlePlayerConductMission(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

BOOL HandlePlayerFairyInspect(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

BOOL HandlePlayerAssassinate(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

BOOL HandlePlayerTextMessage(_Inout_ PCONNECTION_INFO pConnInfo, _In_ yyjson_val* pJsonRoot);

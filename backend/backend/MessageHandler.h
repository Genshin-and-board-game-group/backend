#pragma once
#include "common.h"
#include "HttpSendRecv.h"
#include "yyjson.h"

BOOL HandleCreateRoom(PCONNECTION_INFO pConnInfo, yyjson_val* pJsonRoot);

#include "common.h"
#include "yyjson.h"
#include "HttpSendRecv.h"

BOOL ParseAndDispatchJsonMessage(_In_ PCONNECTION_INFO pConnInfo, _In_ PBYTE pJsonMessage, _In_ ULONG cbMessageLen)
{
    yyjson_doc* JsonDoc = yyjson_read(pJsonMessage, cbMessageLen, 0);
    if (!JsonDoc) return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_val* pType = yyjson_obj_get(JsonDoc->root, "type");
        if (!pType)
            __leave;

        char* pTypeStr = yyjson_get_str(pType);
        if (!pTypeStr)
            __leave;

        printf("type: %s\n", pTypeStr);
        bSuccess = TRUE;
    }
    __finally
    {
        yyjson_doc_free(JsonDoc);
    }
    return bSuccess;
}
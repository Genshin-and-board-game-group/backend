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

        const char* pTypeStr = yyjson_get_str(pType);
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

VOID SendJsonCompleteCallback(_In_ PCONNECTION_INFO pConnInfo, _In_ PWEBSOCK_SEND_BUF pWebsockSendBuf)
{
    free(pWebsockSendBuf->WebsockBuf.Data.pbBuffer); // string allocated from yyjson_mut_write
    HeapFree(GetProcessHeap(), 0, pWebsockSendBuf);
}

BOOL SendJsonMessage(_In_ PCONNECTION_INFO pConnInfo, _In_ yyjson_mut_doc* JsonDoc)
{
    SIZE_T JsonLen;
    BOOL bSuccess = FALSE;
    PWEBSOCK_SEND_BUF pWebsockSendbuf = NULL;

    char* JsonString = yyjson_mut_write(JsonDoc, 0, &JsonLen);
    if (!JsonString)
        return FALSE;

    __try
    {
        pWebsockSendbuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WEBSOCK_SEND_BUF));
        if (!pWebsockSendbuf)
            __leave;

        pWebsockSendbuf->Callback = SendJsonCompleteCallback;
        pWebsockSendbuf->WebsockBuf.Data.pbBuffer = JsonString;
        pWebsockSendbuf->WebsockBuf.Data.ulBufferLength = (ULONG)JsonLen;

        if (!WebsockSendMessage(pConnInfo, pWebsockSendbuf))
            __leave;

        bSuccess = TRUE;
    }
    __finally
    {
        if (!bSuccess)
        {
            if (pWebsockSendbuf)
                HeapFree(GetProcessHeap(), 0, pWebsockSendbuf);
            free(JsonString);
        }
    }
    return bSuccess;
}

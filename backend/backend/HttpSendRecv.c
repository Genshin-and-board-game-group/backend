#include "common.h"
#include "HttpIOPack.h"
#include "HttpSendRecv.h"

static USHORT g_usEntityTooLargeCode = 413;
static CHAR g_szEntityTooLargeReason[] = "Request Entity Too Large";
static CHAR g_szEntityTooLargeMessage[] = "Large buffer support is not implemented";

#define REQUEST_BUFFER_SIZE 4096 // extra buffer we provided store entity etc...

typedef struct _HTTP_RESPONSE_IODATA
{
    HTTP_RESPONSE HttpResponse;
    HTTP_DATA_CHUNK HttpDataChunk;
} HTTP_RESPONSE_IODATA, * PHTTP_RESPONSE_IODATA;

static VOID CALLBACK ServerHTTPCompletionCallback(
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _In_opt_    PVOID                 Context,
    _In_opt_    PVOID                 Overlapped,
    _In_        ULONG                 IoResult,
    _In_        ULONG_PTR             BytesTransferred,
    _Inout_     PTP_IO                Io
);

static BOOL AsyncRecvHttpRequest(VOID);

static BOOL AsyncSendHttpResponse(
    _In_ HTTP_REQUEST_ID RequestID,
    _In_ USHORT StatusCode,
    _In_ PCSTR pGlobalReason,
    _In_ USHORT ReasonLen,
    _In_opt_ PVOID pGlobalBodyBuffer,
    _In_ ULONG BufferLen);

static VOID RecvRequestCallback(
    _In_ PHTTP_IOPACK pHttpIoPack,
    _In_ ULONG IoResult,
    _In_ ULONG_PTR BytesTransferred,
    _Inout_ PTP_IO Io);

static VOID SendResponseCallback(
    _In_ PHTTP_IOPACK pHttpIoPack,
    _In_ ULONG IoResult,
    _In_ ULONG_PTR BytesTransferred,
    _Inout_ PTP_IO Io);

static BOOL bServerRunning = FALSE;
static HANDLE hReqHandle = NULL;
static HTTP_SERVER_SESSION_ID ServerSessionID = 0;
static HTTP_URL_GROUP_ID UrlGroupID = 0;
static PTP_IO pHTTPRequestIO = NULL;

BOOL StartHTTPServer(VOID)
{
    HTTPAPI_VERSION HttpApiVersion = HTTPAPI_VERSION_2;
    ULONG ret;

    BOOL bSuccess = FALSE;

    ret = HttpInitialize(HttpApiVersion, HTTP_INITIALIZE_SERVER, NULL);
    if (ret != NO_ERROR)
    {
        PrintErrorMessage("HttpInitialize", ret);
        return bSuccess;
    }

    bServerRunning = TRUE;
    __try
    {
        ret = HttpCreateRequestQueue(HttpApiVersion, NULL, NULL, 0, &hReqHandle);
        if (ret != NO_ERROR)
        {
            PrintErrorMessage("HttpCreateRequestQueue", ret);
            __leave;
        }

        ret = HttpCreateServerSession(HttpApiVersion, &ServerSessionID, 0);
        if (ret != NO_ERROR)
        {
            PrintErrorMessage("HttpCreateServerSession", ret);
            __leave;
        }

        ret = HttpCreateUrlGroup(ServerSessionID, &UrlGroupID, 0);
        if (ret != NO_ERROR)
        {
            PrintErrorMessage("HttpCreateUrlGroup", ret);
            __leave;
        }

        ret = HttpAddUrlToUrlGroup(UrlGroupID, L"http://+:80/api", 0, 0);
        if (ret != NO_ERROR)
        {
            PrintErrorMessage("HttpAddUrlToUrlGroup", ret);
            __leave;
        }

        HTTP_BINDING_INFO BindingInfo = { 0 };
        BindingInfo.Flags.Present = 1;
        BindingInfo.RequestQueueHandle = hReqHandle;

        ret = HttpSetUrlGroupProperty(UrlGroupID, HttpServerBindingProperty, &BindingInfo, sizeof(BindingInfo));
        if (ret != NO_ERROR)
        {
            PrintErrorMessage("HttpSetUrlGroupProperty", ret);
            __leave;
        }

        // bind to thread pool
        pHTTPRequestIO = CreateThreadpoolIo(hReqHandle, ServerHTTPCompletionCallback, NULL, NULL);
        if (!pHTTPRequestIO)
        {
            PrintErrorMessage("CreateThreadpoolIo", GetLastError());
            __leave;
        }

        // TODO: Launch specified number of calls based on CPU affinity
        if (!AsyncRecvHttpRequest())
        {
            __leave;
        }
        bSuccess = TRUE;
    }
    __finally
    {
        if (!bSuccess)
        {
            StopHTTPServer();
        }
    }
    return bSuccess;
}

VOID StopHTTPServer(VOID)
{
    bServerRunning = FALSE;

    if (pHTTPRequestIO) CloseThreadpoolIo(pHTTPRequestIO);
    if (UrlGroupID) HttpCloseUrlGroup(UrlGroupID);
    if (HttpCloseServerSession) HttpCloseServerSession(ServerSessionID);
    if (hReqHandle)
    {
        HttpShutdownRequestQueue(hReqHandle);
        HttpCloseRequestQueue(hReqHandle);
    }
    HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);

    pHTTPRequestIO = NULL;
    UrlGroupID = 0;
    ServerSessionID = 0;
    hReqHandle = NULL;
}

static VOID CALLBACK ServerHTTPCompletionCallback(
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _In_opt_    PVOID                 Context,
    _In_opt_    PVOID                 Overlapped,
    _In_        ULONG                 IoResult,
    _In_        ULONG_PTR             BytesTransferred,
    _Inout_     PTP_IO                Io
)
{
    if (!Overlapped)
    {
        PrintErrorMessage("ServerHTTPCompletionCallback", IoResult);
        return;
    }

    PHTTP_IOPACK pHttpIoPack = (PHTTP_IOPACK)Overlapped;
    pHttpIoPack->Callback(pHttpIoPack, IoResult, BytesTransferred, Io);
}

static BOOL AsyncRecvHttpRequest(VOID)
{
    PHTTP_IOPACK pHttpIoPack = NULL;
    BOOL bSuccess = FALSE;

    StartThreadpoolIo(pHTTPRequestIO);
    __try
    {
        pHttpIoPack = AllocHttpIOPack(RecvRequestCallback, sizeof(HTTP_REQUEST) + REQUEST_BUFFER_SIZE);
        if (!pHttpIoPack)
            __leave;

        PHTTP_REQUEST pHttpRequest = (PHTTP_REQUEST)(pHttpIoPack + 1);
        ULONG ret = HttpReceiveHttpRequest(hReqHandle, HTTP_NULL_ID, 0, pHttpRequest, sizeof(HTTP_REQUEST) + REQUEST_BUFFER_SIZE, NULL, (LPOVERLAPPED)pHttpIoPack);
        if (ret != NO_ERROR && ret != ERROR_IO_PENDING)
        {
            PrintErrorMessage("HttpReceiveHttpRequest", ret);
            __leave;
        }
        bSuccess = TRUE;
    }
    __finally
    {
        if (!bSuccess)
        {
            if (pHttpIoPack) FreeHttpIOPack(pHttpIoPack);
            CancelThreadpoolIo(pHTTPRequestIO);
        }
    }
    return bSuccess;
}

static BOOL AsyncSendHttpResponse(
    _In_ HTTP_REQUEST_ID RequestID,
    _In_ USHORT StatusCode,
    _In_ PCSTR pGlobalReason,
    _In_ USHORT ReasonLen,
    _In_opt_ PVOID pGlobalBodyBuffer,
    _In_ ULONG BufferLen)
{
    PHTTP_IOPACK pHttpIoPack = NULL;
    BOOL bSuccess = FALSE;

    StartThreadpoolIo(pHTTPRequestIO);
    __try
    {
        pHttpIoPack = AllocHttpIOPack(SendResponseCallback, sizeof(HTTP_RESPONSE_IODATA));
        if (!pHttpIoPack)__leave;

        PHTTP_RESPONSE_IODATA pData = (PHTTP_RESPONSE_IODATA)(pHttpIoPack + 1);

        pData->HttpResponse.StatusCode = StatusCode;
        pData->HttpResponse.pReason = pGlobalReason;
        pData->HttpResponse.ReasonLength = ReasonLen;

        if (pGlobalBodyBuffer)
        {
            pData->HttpDataChunk.DataChunkType = HttpDataChunkFromMemory;
            pData->HttpDataChunk.FromMemory.pBuffer = pGlobalBodyBuffer;
            pData->HttpDataChunk.FromMemory.BufferLength = BufferLen;
            pData->HttpResponse.EntityChunkCount = 1;
            pData->HttpResponse.pEntityChunks = &pData->HttpDataChunk;
        }

        ULONG ret = HttpSendHttpResponse(hReqHandle, RequestID, 0, &pData->HttpResponse, NULL, NULL, NULL, 0, (LPOVERLAPPED)pHttpIoPack, NULL);
        if (ret != NO_ERROR && ret != ERROR_IO_PENDING)
        {
            PrintErrorMessage("HttpReceiveHttpRequest", ret);
            __leave;
        }
        bSuccess = TRUE;
    }
    __finally
    {
        if (!bSuccess)
        {
            if (pHttpIoPack) FreeHttpIOPack(pHttpIoPack);
            CancelThreadpoolIo(pHTTPRequestIO);
        }
    }

    return bSuccess;
}

static VOID RecvRequestCallback(
    _In_ PHTTP_IOPACK pHttpIoPack,
    _In_ ULONG IoResult,
    _In_ ULONG_PTR BytesTransferred,
    _Inout_ PTP_IO Io)
{
    if (bServerRunning)
    {
        PHTTP_REQUEST pHttpRequest = (PHTTP_REQUEST)(pHttpIoPack + 1);
        switch (IoResult)
        {
        case NO_ERROR:
            // TODO: following code is just a placeholder
            AsyncSendHttpResponse(
                pHttpRequest->RequestId,
                200,
                "ok",
                (USHORT)strlen("ok"),
                "You hit the server!",
                (USHORT)strlen("You hit the server!"));

            break;
        case ERROR_MORE_DATA:

            AsyncSendHttpResponse(
                pHttpRequest->RequestId,
                g_usEntityTooLargeCode,
                g_szEntityTooLargeReason,
                (USHORT)strlen(g_szEntityTooLargeReason),
                g_szEntityTooLargeMessage, 
                (USHORT)strlen(g_szEntityTooLargeMessage));

            break;
        }

        AsyncRecvHttpRequest();
    }
    FreeHttpIOPack(pHttpIoPack);
}

static VOID SendResponseCallback(
    _In_ PHTTP_IOPACK pHttpIoPack,
    _In_ ULONG IoResult,
    _In_ ULONG_PTR BytesTransferred,
    _Inout_ PTP_IO Io)
{
    if (IoResult != NO_ERROR)
    {
        PrintErrorMessage("SendResponseCallback", IoResult);
    }
    FreeHttpIOPack(pHttpIoPack);
}

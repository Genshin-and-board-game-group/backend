#include "common.h"
#include "HttpIOPack.h"
#include "HttpSendRecv.h"

static USHORT g_usSwitchingProtocolsCode = 101;
static CHAR g_szSwitchingProtocolsReason[] = "Switching Protocols";

static USHORT g_usEntityTooLargeCode = 413;
static CHAR g_szEntityTooLargeReason[] = "Request Entity Too Large";
static CHAR g_szEntityTooLargeMessage[] = "Large buffer support is not implemented.";

static USHORT g_usUpgradeRequiredCode = 426;
static CHAR g_szUpgradeRequiredReason[] = "Upgrade Required";
static CHAR g_szUpgradeRequiredMessage[] = "This API only supports websocket. Upgrade required.";


#define REQUEST_BUFFER_SIZE 4096 // extra buffer we provided store entity etc...

typedef struct _HTTP_RESPONSE_IODATA
{
    HTTP_RESPONSE HttpResponse;
    HTTP_DATA_CHUNK HttpDataChunk;
} HTTP_RESPONSE_IODATA, * PHTTP_RESPONSE_IODATA;

typedef struct _HTTP_UPGRADE_WS_IODATA
{
    HTTP_RESPONSE HttpResponse;
    WEB_SOCKET_HANDLE hWebSock;
} HTTP_UPGRADE_WS_IODATA, * PHTTP_UPGRADE_WS_IODATA;

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

static BOOL AsyncSendUpgradeToWebsocket(_In_ PHTTP_REQUEST pHttpRequest);

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

static VOID SendUpgradeWebsockCallback(
    _In_ PHTTP_IOPACK pHttpIoPack,
    _In_ ULONG IoResult,
    _In_ ULONG_PTR BytesTransferred,
    _Inout_ PTP_IO Io);

static BOOL bServerRunning = FALSE;
static HANDLE hReqHandle = NULL;
static HTTP_SERVER_SESSION_ID ServerSessionID = 0;
static HTTP_URL_GROUP_ID UrlGroupID = 0;
static PTP_IO pHTTPRequestIO = NULL;

BOOL StartHTTPServer(DWORD RequestCount)
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
        for (DWORD i = 0; i < RequestCount; i++)
        {
            if (!AsyncRecvHttpRequest())
            {
                __leave;
            }
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
            PrintErrorMessage("HttpSendHttpResponse", ret);
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

static BOOL AsyncSendUpgradeToWebsocket(_In_ PHTTP_REQUEST pHttpRequest)
{
    PWEB_SOCKET_HTTP_HEADER pWebSockReqHeaders = NULL;
    WEB_SOCKET_HANDLE serverHandle = NULL;

    ULONG serverAdditionalHeaderCount = 0;
    WEB_SOCKET_HTTP_HEADER* serverAdditionalHeaders = NULL;
    HRESULT hr = S_OK;
    PHTTP_IOPACK pHttpIoPack = NULL;
    PHTTP_UNKNOWN_HEADER pUnknownHeaders = NULL;

    BOOL bSuccess = FALSE;
    __try
    {
        hr = WebSocketCreateServerHandle(NULL, 0, &serverHandle);
        if (FAILED(hr))
            __leave;

        // Copy headers received to pass to WebSocketBeginServerHandshake.
        // the following header will be copied:
        // Connection Upgrade Host
        // and all headers in unknown headers.
        pWebSockReqHeaders = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (3 + (SIZE_T)pHttpRequest->Headers.UnknownHeaderCount) * sizeof(WEB_SOCKET_HTTP_HEADER));
        if (!pWebSockReqHeaders) __leave;

        static CHAR szConnection[] = "Connection";
        static CHAR szUpgrade[] = "Upgrade";
        static CHAR szHost[] = "Host";
        pWebSockReqHeaders[0].pcName = szConnection;
        pWebSockReqHeaders[0].ulNameLength = _countof(szConnection) - 1;
        pWebSockReqHeaders[0].pcValue = pHttpRequest->Headers.KnownHeaders[HttpHeaderConnection].pRawValue;
        pWebSockReqHeaders[0].ulValueLength = pHttpRequest->Headers.KnownHeaders[HttpHeaderConnection].RawValueLength;

        pWebSockReqHeaders[1].pcName = szUpgrade;
        pWebSockReqHeaders[1].ulNameLength = _countof(szUpgrade) - 1;
        pWebSockReqHeaders[1].pcValue = pHttpRequest->Headers.KnownHeaders[HttpHeaderUpgrade].pRawValue;
        pWebSockReqHeaders[1].ulValueLength = pHttpRequest->Headers.KnownHeaders[HttpHeaderUpgrade].RawValueLength;

        pWebSockReqHeaders[2].pcName = szHost;
        pWebSockReqHeaders[2].ulNameLength = _countof(szHost) - 1;
        pWebSockReqHeaders[2].pcValue = pHttpRequest->Headers.KnownHeaders[HttpHeaderHost].pRawValue;
        pWebSockReqHeaders[2].ulValueLength = pHttpRequest->Headers.KnownHeaders[HttpHeaderHost].RawValueLength;

        for (USHORT i = 0; i < pHttpRequest->Headers.UnknownHeaderCount; i++)
        {
            pWebSockReqHeaders[i + 3].pcName = pHttpRequest->Headers.pUnknownHeaders[i].pName;
            pWebSockReqHeaders[i + 3].ulNameLength = pHttpRequest->Headers.pUnknownHeaders[i].NameLength;
            pWebSockReqHeaders[i + 3].pcValue = pHttpRequest->Headers.pUnknownHeaders[i].pRawValue;
            pWebSockReqHeaders[i + 3].ulValueLength = pHttpRequest->Headers.pUnknownHeaders[i].RawValueLength;
        }

        hr = WebSocketBeginServerHandshake(
            serverHandle,
            NULL,
            NULL,
            0,
            pWebSockReqHeaders,
            3 + pHttpRequest->Headers.UnknownHeaderCount,
            &serverAdditionalHeaders,
            &serverAdditionalHeaderCount);

        if (FAILED(hr))
            __leave;

        pHttpIoPack = AllocHttpIOPack(SendUpgradeWebsockCallback, sizeof(HTTP_UPGRADE_WS_IODATA));
        if (!pHttpIoPack)
            __leave;

        PHTTP_UPGRADE_WS_IODATA pData = (PHTTP_UPGRADE_WS_IODATA)(pHttpIoPack + 1);

        pData->HttpResponse.StatusCode = g_usSwitchingProtocolsCode;
        pData->HttpResponse.pReason = g_szSwitchingProtocolsReason;
        pData->HttpResponse.ReasonLength = _countof(g_szSwitchingProtocolsReason) - 1;

        pUnknownHeaders = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, serverAdditionalHeaderCount * sizeof(HTTP_UNKNOWN_HEADER));
        if (!pUnknownHeaders)
            __leave;

        // TODO: the code below is a little bit hacky
        // I have no idea but HttpSendHttpResponse eats my "Connection" header
        // so I have to add it into unknown headers.
        // and if I don't specify "Upgrade" header in known headers, HttpSendHttpResponse will return 87 ERROR_INVALID_PARAMETER
        INT AddedUnknownHeadersCnt = 0;
        for (int i = 0; i < serverAdditionalHeaderCount; i++)
        {
            static char szUpgrade[] = "Upgrade";
            if (_strnicmp(serverAdditionalHeaders[i].pcName, szUpgrade, _countof(szUpgrade) - 1) == 0) // Don't add "Upgrade" header twice. we will add it known headers later.
                continue;

            pUnknownHeaders[AddedUnknownHeadersCnt].NameLength = serverAdditionalHeaders[i].ulNameLength;
            pUnknownHeaders[AddedUnknownHeadersCnt].pName = serverAdditionalHeaders[i].pcName;
            pUnknownHeaders[AddedUnknownHeadersCnt].RawValueLength = serverAdditionalHeaders[i].ulValueLength;
            pUnknownHeaders[AddedUnknownHeadersCnt].pRawValue = serverAdditionalHeaders[i].pcValue;
            AddedUnknownHeadersCnt++;
        }
        pData->HttpResponse.Headers.pUnknownHeaders = pUnknownHeaders;
        pData->HttpResponse.Headers.UnknownHeaderCount = AddedUnknownHeadersCnt;

        static char szWebSocket[] = "WebSocket";
        pData->HttpResponse.Headers.KnownHeaders[HttpHeaderUpgrade].pRawValue = szWebSocket;
        pData->HttpResponse.Headers.KnownHeaders[HttpHeaderUpgrade].RawValueLength = _countof(szWebSocket) - 1;

        pData->hWebSock = serverHandle;

        ULONG ret = HttpSendHttpResponse(
            hReqHandle,
            pHttpRequest->RequestId,
            HTTP_SEND_RESPONSE_FLAG_OPAQUE | HTTP_SEND_RESPONSE_FLAG_MORE_DATA,
            &pData->HttpResponse,
            NULL,
            NULL,
            NULL,
            0,
            (LPOVERLAPPED)pHttpIoPack,
            NULL);
        if (ret != NO_ERROR && ret != ERROR_IO_PENDING)
        {
            PrintErrorMessage("HttpSendHttpResponse", ret);
            __leave;
        }
        bSuccess = TRUE;
    }
    __finally
    {
        if (pWebSockReqHeaders) HeapFree(GetProcessHeap(), 0, pWebSockReqHeaders);

        if (!bSuccess)
        {
            if (serverAdditionalHeaders)
            {
                WebSocketEndServerHandshake(serverHandle);
            }
            if (serverHandle)
            {
                WebSocketDeleteHandle(serverHandle);
            }
            if (pHttpIoPack)
            {
                FreeHttpIOPack(pHttpIoPack);
            }
            if (pUnknownHeaders)
            {
                HeapFree(GetProcessHeap(), 0, pUnknownHeaders);
            }
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
        {
            BOOL bSuccess = AsyncSendUpgradeToWebsocket(pHttpRequest);

            if (!bSuccess)
            {
                AsyncSendHttpResponse(
                    pHttpRequest->RequestId,
                    g_usUpgradeRequiredCode,
                    g_szUpgradeRequiredReason,
                    (USHORT)strlen(g_szUpgradeRequiredReason),
                    g_szUpgradeRequiredMessage,
                    (USHORT)strlen(g_szUpgradeRequiredMessage));
            }

            break;
        }
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

static VOID SendUpgradeWebsockCallback(
    _In_ PHTTP_IOPACK pHttpIoPack,
    _In_ ULONG IoResult,
    _In_ ULONG_PTR BytesTransferred,
    _Inout_ PTP_IO Io)
{
    PHTTP_UPGRADE_WS_IODATA pData = (PHTTP_UPGRADE_WS_IODATA)(pHttpIoPack + 1);
    BOOL bCleanup = FALSE;
    if (bServerRunning)
    {
        if (IoResult == NO_ERROR)
        {
            // connection established successfully
            // TODO:
            printf("a player connected\n");
        }
        else
        {
            PrintErrorMessage("SendUpgradeWebsockCallback", IoResult);
            bCleanup = TRUE;
        }
    }
    else
    {
        bCleanup = TRUE;
    }
    HeapFree(GetProcessHeap(), 0, pData->HttpResponse.Headers.pUnknownHeaders);
    WebSocketEndServerHandshake(pData->hWebSock);
    if (bCleanup)
    {
        WebSocketDeleteHandle(pData->hWebSock);
    }
    FreeHttpIOPack(pHttpIoPack);
}

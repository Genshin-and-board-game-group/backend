#include "common.h"
#include "RecvRequestHandler.h"

#pragma comment(lib, "httpapi.lib")
#pragma comment(lib, "Websocket.lib")

#define REQUEST_BUFFER_SIZE 4096 // extra buffer we provided store entity etc...

VOID PrintErrorMessage(
    _In_opt_ LPCSTR ErrorMessage,
    _In_ DWORD dwError);

BOOL StartHTTPServer(VOID);

VOID StopHTTPServer(VOID);

VOID CALLBACK ServerHTTPCompletionCallback(
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _In_opt_    PVOID                 Context,
    _In_opt_    PVOID                 Overlapped,
    _In_        ULONG                 IoResult,
    _In_        ULONG_PTR             BytesTransferred,
    _Inout_     PTP_IO                Io
);

BOOL bServerRunning = FALSE;
HANDLE hReqHandle = NULL;
HTTP_SERVER_SESSION_ID ServerSessionID = 0;
HTTP_URL_GROUP_ID UrlGroupID = 0;
PTP_IO pHTTPRequestIO = NULL;

VOID PrintErrorMessage(
    _In_opt_ LPCSTR ErrorMessage,
    _In_ DWORD dwError)
{
    HLOCAL hlocal = NULL;   // Buffer that gets the error message string

    // Use the default system locale since we look for Windows messages.
    // Note: this MAKELANGID combination has 0 as value
    DWORD systemLocale = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    // Get the error code's textual description
    BOOL fOk = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, dwError, systemLocale,
        (PSTR)&hlocal, 0, NULL);

    if (!fOk)
    {
        // Is it a network-related error?
        HMODULE hDll = LoadLibraryEx(TEXT("netmsg.dll"), NULL,
            DONT_RESOLVE_DLL_REFERENCES);

        if (hDll != NULL)
        {
            fOk = FormatMessageA(
                FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS |
                FORMAT_MESSAGE_ALLOCATE_BUFFER,
                hDll, dwError, systemLocale,
                (PSTR)&hlocal, 0, NULL);
            FreeLibrary(hDll);
        }
    }

    if (fOk && (hlocal != NULL))
    {
        if (ErrorMessage) printf("%s: ", ErrorMessage);
        printf("%s", (PCSTR)LocalLock(hlocal));
        LocalFree(hlocal);
    }
    else
    {
        printf("No text found for this error number.");
    }
}

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

VOID CALLBACK ServerHTTPCompletionCallback(
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

BOOL LaunchRecvHTTPRequest(VOID)
{
    PHTTP_IOPACK pHttpIoPack = NULL;
    BOOL bSuccess = FALSE;

    StartThreadpoolIo(pHTTPRequestIO);

    __try
    {
        PHTTP_IOPACK pHttpIoPack = AllocHttpIOPack(RecvRequestCallback, sizeof(HTTP_REQUEST) + REQUEST_BUFFER_SIZE);
        if (!pHttpIoPack)
            __leave;

        PHTTP_REQUEST pHttpRequest = (PHTTP_REQUEST)(pHttpIoPack + 1);
        ULONG ret = HttpReceiveHttpRequest(hReqHandle, HTTP_NULL_ID, 0, pHttpRequest, sizeof(HTTP_REQUEST) + REQUEST_BUFFER_SIZE, NULL, (LPOVERLAPPED)pHttpIoPack);
        if (ret != ERROR_IO_PENDING)
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

int main()
{
    if (!StartHTTPServer())
    {
        return 1;
    }
    if(!LaunchRecvHTTPRequest())
    {
        return 1;
    }
    while (1)
    {
        char command[128] = { 0 };
        scanf_s("%s", command, (UINT)_countof(command));

        if (strcmp(command, "stop") == 0)
        {
            break;
        }
        printf("unknown command: %s\n", command);
    }
    StopHTTPServer();
    return 0;
}

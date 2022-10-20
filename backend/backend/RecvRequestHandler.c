#include "common.h"
#include "RecvRequestHandler.h"

VOID RecvRequestCallback(
    _In_ PHTTP_IOPACK pHttpIoPack,
    _In_ ULONG IoResult,
    _In_ ULONG_PTR BytesTransferred,
    _Inout_ PTP_IO Io)
{
    // TODO...
    FreeHttpIOPack(pHttpIoPack);
}

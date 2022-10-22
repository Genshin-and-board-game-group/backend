#include "common.h"
#include "HttpIOPack.h"

_Ret_maybenull_
PHTTP_IOPACK AllocHttpIOPack(
    _In_ HTTP_COMPLETION_FUNCTION Callback,
    _In_ SIZE_T ExtSize)
{
    PHTTP_IOPACK pPack = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HTTP_IOPACK) + ExtSize);
    if (!pPack) return NULL;
    pPack->Callback = Callback;
    return pPack;
}

VOID FreeHttpIOPack(_In_ _Frees_ptr_ PHTTP_IOPACK pHttpIOPack)
{
    HeapFree(GetProcessHeap(), 0, pHttpIOPack);
}

#pragma once
#include "common.h"

typedef struct _HTTP_IOPACK HTTP_IOPACK, * PHTTP_IOPACK;

typedef VOID(*HTTP_COMPLETION_FUNCTION)(PHTTP_IOPACK pHttpIoPack, ULONG IoResult, ULONG_PTR BytesTransferred, PTP_IO Io);

typedef struct _HTTP_IOPACK
{
    OVERLAPPED Overlapped;
    HTTP_COMPLETION_FUNCTION Callback;
} HTTP_IOPACK, * PHTTP_IOPACK;

_Ret_maybenull_
PHTTP_IOPACK AllocHttpIOPack(
    _In_ HTTP_COMPLETION_FUNCTION Callback,
    _In_ SIZE_T ExtSize);

VOID FreeHttpIOPack(_In_ _Frees_ptr_ PHTTP_IOPACK pHttpIOPack);

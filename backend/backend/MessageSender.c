#include "common.h"
#include "yyjson.h"
#include "JsonHandler.h"

BOOL SendCreateRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_ UINT RoomNum, _In_ UINT ID, _In_opt_z_ CHAR* Reason)
{
    char szRoomNumber[11] = { 0 }; // 2^32 tooks 10 char to store under decimal

    // Create a mutable doc
    yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
    if (!doc)
        return FALSE;

    BOOL bSuccess = FALSE;
    __try
    {
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        if (!root)
            __leave;
        yyjson_mut_doc_set_root(doc, root);
        yyjson_mut_obj_add_str(doc, root, "type", "createRoom");
        yyjson_mut_obj_add_str(doc, root, "result", bResult ? "success" : "fail");

        if (bResult)
        {
            sprintf_s(szRoomNumber, _countof(szRoomNumber), "%d", RoomNum);
            yyjson_mut_obj_add_str(doc, root, "roomNumber", szRoomNumber);
            yyjson_mut_obj_add_sint(doc, root, "ID", ID);
        }
        else
        {
            yyjson_mut_obj_add_str(doc, root, "reason", Reason);
        }

        bSuccess = SendJsonMessage(pConnInfo, doc);
    }
    __finally
    {
        // Free the doc
        yyjson_mut_doc_free(doc);
    }
    return bSuccess;
}
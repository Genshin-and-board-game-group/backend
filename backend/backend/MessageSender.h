#pragma once
#include "common.h"
#include "HttpSendRecv.h"

typedef struct _HINTLIST
{
    UINT ID;
    UINT HintType;
}HINTLIST, *PHINTLIST;

typedef struct _VOTELIST
{
    UINT ID;
    BOOL VoteResult;
}VOTELIST, *PVOTELIST;

/// <summary>
/// �ظ���ҷ��͵Ĵ�������
/// </summary>
/// <param name="pConnInfo">������Ϣ�������Ϣ</param>
/// <param name="bResult">�ɹ�������Ϣ���</param>
/// <param name="RoomNum">����ţ����� bResult Ϊ TRUE ʱʹ��</param>
/// <param name="ID">������ ID������ bResult Ϊ TRUE ʱʹ��</param>
/// <param name="Reason">����ʧ��ԭ�򡣽��� bResult Ϊ FALSE ʱʹ��</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL ReplyCreateRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_ UINT RoomNum, _In_ UINT ID, _In_opt_z_ CHAR* Reason);

/// <summary>
/// �ظ���ҷ��͵ļ��뷿��
/// </summary>
/// <param name="pConnInfo">������Ϣ�������Ϣ</param>
/// <param name="bResult">�ɹ�������Ϣ���</param>
/// <param name="ID">������ ID������ bResult Ϊ TRUE ʱʹ��</param>
/// <param name="Reason">����ʧ��ԭ�򡣽��� bResult Ϊ FALSE ʱʹ��</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL ReplyJoinRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_ UINT ID, _In_opt_z_ CHAR* Reason);

/// <summary>
/// �ظ���ҷ��͵��˳�����
/// </summary>
/// <param name="pConnInfo">������Ϣ�������Ϣ</param>
/// <param name="bResult">�ɹ�������Ϣ���</param>
/// <param name="Reason">�˳�ʧ��ԭ�򡣽��� bResult Ϊ FALSE ʱʹ��</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL ReplyLeaveRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// �ظ���ҷ��͵Ŀ�ʼ��Ϸ
/// </summary>
/// <param name="pConnInfo">������Ϣ�������Ϣ</param>
/// <param name="bResult">�ɹ�������Ϣ���</param>
/// <param name="Reason">��ʼ��Ϸʧ��ԭ�򡣽��� bResult Ϊ FALSE ʱʹ��</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL ReplyStartGame(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// ����֪ͨ�����Ϸ��ʼ
/// </summary>
/// <param name="pConnInfo">������Ϣ�������Ϣ</param>
/// <param name="Role">��ҵĽ�ɫ���� ROLE_MERLIN</param>
/// <param name="bFairyEnabled">��������Ů�Ƿ�����</param>
/// <param name="FairyID">����ʱ��Ů�� ID</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL SendBeginGame(_In_ PCONNECTION_INFO pConnInfo, _In_ UINT Role, _In_ BOOL bFairyEnabled, _In_ UINT FairyID);

/// <summary>
/// ����֪ͨ����յ��й���һ��/һЩ��ҵ�����
/// </summary>
/// <param name="pConnInfo">������Ϣ�������Ϣ</param>
/// <param name="HintCnt">����������</param>
/// <param name="HintList">�����б�</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL SendRoleHint(_In_ PCONNECTION_INFO pConnInfo, _In_ UINT HintCnt, _In_ HINTLIST HintList[]);

/// <summary>
/// �ظ���ң��ӳ������͵�ѡ����
/// </summary>
/// <param name="pConnInfo">������Ϣ�������Ϣ</param>
/// <param name="bResult">�ɹ�������Ϣ���</param>
/// <param name="Reason">ʧ��ԭ�򡣽��� bResult Ϊ FALSE ʱʹ��</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL ReplyPlayerSelectTeam(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// �ظ���ң��ӳ������͵�ȷ�ϱ��
/// </summary>
/// <param name="pConnInfo">������Ϣ�������Ϣ</param>
/// <param name="bResult">�ɹ�������Ϣ���</param>
/// <param name="Reason">ʧ��ԭ�򡣽��� bResult Ϊ FALSE ʱʹ��</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL ReplyPlayerConfirmTeam(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// �ظ���ҷ��͵�ͶƱ�޳�/���Ա�ӳ�Ա��������
/// </summary>
/// <param name="pConnInfo">������Ϣ�������Ϣ</param>
/// <param name="bResult">�ɹ�������Ϣ���</param>
/// <param name="Reason">ʧ��ԭ�򡣽��� bResult Ϊ FALSE ʱʹ��</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL ReplyPlayerVoteTeam(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// �ظ���ҷ��͵�ѡ��ִ�л����ƻ�����
/// </summary>
/// <param name="pConnInfo">������Ϣ�������Ϣ</param>
/// <param name="bResult">�ɹ�������Ϣ���</param>
/// <param name="Reason">ʧ��ԭ�򡣽��� bResult Ϊ FALSE ʱʹ��</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL ReplyPlayerConductMission(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// �ظ���ҷ��͵���Ů����
/// </summary>
/// <param name="pConnInfo">������Ϣ�������Ϣ</param>
/// <param name="bResult">�ɹ�������Ϣ���</param>
/// <param name="Reason">ʧ��ԭ�򡣽��� bResult Ϊ FALSE ʱʹ��</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL ReplyPlayerFairyInspect(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// �ظ���ң��̿ͣ����͵Ĵ�ɱѡ��
/// </summary>
/// <param name="pConnInfo">������Ϣ�������Ϣ</param>
/// <param name="bResult">�ɹ�������Ϣ���</param>
/// <param name="Reason">ʧ��ԭ�򡣽��� bResult Ϊ FALSE ʱʹ��</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL ReplyPlayerAssassinate(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// �ظ��յ���ҷ��͵�������Ϣ
/// </summary>
/// <param name="pConnInfo">������Ϣ�������Ϣ</param>
/// <param name="bResult">�ɹ�������Ϣ���</param>
/// <param name="Reason">ʧ��ԭ�򡣽��� bResult Ϊ FALSE ʱʹ��</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL ReplyPlayerTextMessage(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// ��������ҷ���������ҵ�״̬
/// </summary>
/// <param name="pRoom">�㲥�ķ���</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL BroadcastRoomStatus(_In_ PGAME_ROOM pRoom);

/// <summary>
/// ��������ҷ��Ͷӳ����
/// </summary>
/// <param name="pRoom">�㲥�ķ���</param>
/// <param name="ID">�µĶӳ��� ID</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL BroadcastSetLeader(_In_ PGAME_ROOM pRoom, _In_ UINT ID);

/// <summary>
/// ��������ҷ��Ͷӳ�ѡ��Ķ�Ա
/// </summary>
/// <param name="pRoom">�㲥�ķ���</param>
/// <param name="TeamSize">��ǰ�ӳ�ѡ��Ķ�������</param>
/// <param name="TeamArr">��ǰ�ӳ�ѡ��Ķ�����Ա�� ID</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL BroadcastSelectTeam(_In_ PGAME_ROOM pRoom, _In_ UINT TeamSize, _In_ UINT32 TeamArr[]);

/// <summary>
/// ��������ҷ��Ͷӳ�ȷ���˱��
/// </summary>
/// <param name="pRoom">�㲥�ķ���</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL BroadcastConfirmTeam(_In_ PGAME_ROOM pRoom);

/// <summary>
/// ��������ҷ��͵�ǰͶƱ����
/// </summary>
/// <param name="pRoom">�㲥�ķ���</param>
/// <param name="VotedCnt">�Ѿ�ͶƱ������</param>
/// <param name="VotedIDList">�Ѿ�ͶƱ����� ID</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL BroadcastVoteTeamProgress(_In_ PGAME_ROOM pRoom, _In_ UINT VotedCnt, _In_ UINT32 VotedIDList[]);

/// <summary>
/// ��������ҷ���ͶƱ���
/// </summary>
/// <param name="pRoom">�㲥�ķ���</param>
/// <param name="bVoteResult">ͶƱ�Ľ����ͨ�����ǲ�ͨ����</param>
/// <param name="VoteListCnt">ͶƱ������</param>
/// <param name="VoteList">ÿ���˵�ͶƱ���</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL BroadcastVoteTeam(_In_ PGAME_ROOM pRoom, _In_ BOOL bVoteResult, _In_ UINT VoteListCnt, _In_ VOTELIST VoteList[]);

/// <summary>
/// ��������ҷ�������ִ�н���
/// </summary>
/// <param name="pRoom">�㲥�ķ���</param>
/// <param name="DecidedCnt">�����������������</param>
/// <param name="DecidedIDList">�������������</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL BroadcastMissionResultProgress(_In_ PGAME_ROOM pRoom, _In_ UINT DecidedCnt, _In_ UINT32 DecidedIDList[]);

/// <summary>
/// ��������ҷ�������ִ�н��
/// </summary>
/// <param name="pRoom">�㲥�ķ���</param>
/// <param name="bMissionSuccess">����ִ�гɹ����</param>
/// <param name="Perform">ִ������������������Ʊ������</param>
/// <param name="Screw">�ƻ�����������������Ʊ������</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL BroadcastMissionResult(_In_ PGAME_ROOM pRoom, _In_ BOOL bMissionSuccess, _In_ UINT32 Perform, _In_ UINT32 Screw);

/// <summary>
/// ��������ҷ�����Ůѡ�������
/// </summary>
/// <param name="pRoom">�㲥�ķ���</param>
/// <param name="InspectID">��Ůѡ������˵� ID</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL BroadcastFairyInspect(_In_ PGAME_ROOM pRoom, _In_ UINT InspectID);

/// <summary>
/// ��������ҷ��ͱ�����Ϸ����
/// </summary>
/// <param name="pRoom">�㲥�ķ���</param>
/// <param name="bWin">���˷��Ƿ�ʤ��</param>
/// <param name="Reason">ʤ��/ʧ��ԭ��</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL BroadcastEndGame(_In_ PGAME_ROOM pRoom, _In_ BOOL bWin, _In_z_ CHAR Reason[]);

/// <summary>
/// ��������ҷ�����ҷ��͵�������Ϣ
/// </summary>
/// <param name="pRoom">�㲥�ķ���</param>
/// <param name="ID">����������Ϣ����ҵ� ID</param>
/// <param name="Message">������Ϣ��UTF8 ����</param>
/// <returns>�Ƿ�������������������ϲ������롣</returns>
BOOL BroadcastTextMessage(_In_ PGAME_ROOM pRoom, _In_ UINT ID, _In_z_ CHAR Message[]);

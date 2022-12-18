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
/// 回复玩家发送的创建房间
/// </summary>
/// <param name="pConnInfo">接收消息的玩家信息</param>
/// <param name="bResult">成功处理消息与否</param>
/// <param name="RoomNum">房间号，仅当 bResult 为 TRUE 时使用</param>
/// <param name="ID">房间内 ID，仅当 bResult 为 TRUE 时使用</param>
/// <param name="Reason">创建失败原因。仅当 bResult 为 FALSE 时使用</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL ReplyCreateRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_ UINT RoomNum, _In_ UINT ID, _In_opt_z_ CHAR* Reason);

/// <summary>
/// 回复玩家发送的加入房间
/// </summary>
/// <param name="pConnInfo">接收消息的玩家信息</param>
/// <param name="bResult">成功处理消息与否</param>
/// <param name="ID">房间内 ID，仅当 bResult 为 TRUE 时使用</param>
/// <param name="Reason">加入失败原因。仅当 bResult 为 FALSE 时使用</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL ReplyJoinRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_ UINT ID, _In_opt_z_ CHAR* Reason);

/// <summary>
/// 回复玩家发送的退出房间
/// </summary>
/// <param name="pConnInfo">接收消息的玩家信息</param>
/// <param name="bResult">成功处理消息与否</param>
/// <param name="Reason">退出失败原因。仅当 bResult 为 FALSE 时使用</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL ReplyLeaveRoom(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// 回复玩家发送的开始游戏
/// </summary>
/// <param name="pConnInfo">接收消息的玩家信息</param>
/// <param name="bResult">成功处理消息与否</param>
/// <param name="Reason">开始游戏失败原因。仅当 bResult 为 FALSE 时使用</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL ReplyStartGame(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// 用于通知玩家游戏开始
/// </summary>
/// <param name="pConnInfo">接收消息的玩家信息</param>
/// <param name="Role">玩家的角色，如 ROLE_MERLIN</param>
/// <param name="bFairyEnabled">本局中仙女是否启用</param>
/// <param name="FairyID">开局时仙女的 ID</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL SendBeginGame(_In_ PCONNECTION_INFO pConnInfo, _In_ UINT Role, _In_ BOOL bFairyEnabled, _In_ UINT FairyID);

/// <summary>
/// 用于通知玩家收到有关另一个/一些玩家的线索
/// </summary>
/// <param name="pConnInfo">接收消息的玩家信息</param>
/// <param name="HintCnt">线索的数量</param>
/// <param name="HintList">线索列表</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL SendRoleHint(_In_ PCONNECTION_INFO pConnInfo, _In_ UINT HintCnt, _In_ HINTLIST HintList[]);

/// <summary>
/// 回复玩家（队长）发送的选择编队
/// </summary>
/// <param name="pConnInfo">接收消息的玩家信息</param>
/// <param name="bResult">成功处理消息与否</param>
/// <param name="Reason">失败原因。仅当 bResult 为 FALSE 时使用</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL ReplyPlayerSelectTeam(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// 回复玩家（队长）发送的确认编队
/// </summary>
/// <param name="pConnInfo">接收消息的玩家信息</param>
/// <param name="bResult">成功处理消息与否</param>
/// <param name="Reason">失败原因。仅当 bResult 为 FALSE 时使用</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL ReplyPlayerConfirmTeam(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// 回复玩家发送的投票赞成/反对编队成员发起任务
/// </summary>
/// <param name="pConnInfo">接收消息的玩家信息</param>
/// <param name="bResult">成功处理消息与否</param>
/// <param name="Reason">失败原因。仅当 bResult 为 FALSE 时使用</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL ReplyPlayerVoteTeam(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// 回复玩家发送的选择执行还是破坏任务
/// </summary>
/// <param name="pConnInfo">接收消息的玩家信息</param>
/// <param name="bResult">成功处理消息与否</param>
/// <param name="Reason">失败原因。仅当 bResult 为 FALSE 时使用</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL ReplyPlayerConductMission(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// 回复玩家发送的仙女验人
/// </summary>
/// <param name="pConnInfo">接收消息的玩家信息</param>
/// <param name="bResult">成功处理消息与否</param>
/// <param name="Reason">失败原因。仅当 bResult 为 FALSE 时使用</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL ReplyPlayerFairyInspect(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// 回复玩家（刺客）发送的刺杀选择
/// </summary>
/// <param name="pConnInfo">接收消息的玩家信息</param>
/// <param name="bResult">成功处理消息与否</param>
/// <param name="Reason">失败原因。仅当 bResult 为 FALSE 时使用</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL ReplyPlayerAssassinate(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// 回复收到玩家发送的聊天消息
/// </summary>
/// <param name="pConnInfo">接收消息的玩家信息</param>
/// <param name="bResult">成功处理消息与否</param>
/// <param name="Reason">失败原因。仅当 bResult 为 FALSE 时使用</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL ReplyPlayerTextMessage(_In_ PCONNECTION_INFO pConnInfo, _In_ BOOL bResult, _In_opt_z_ CHAR* Reason);

/// <summary>
/// 向所有玩家发送所有玩家的状态
/// </summary>
/// <param name="pRoom">广播的房间</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL BroadcastRoomStatus(_In_ PGAME_ROOM pRoom);

/// <summary>
/// 向所有玩家发送队长变更
/// </summary>
/// <param name="pRoom">广播的房间</param>
/// <param name="ID">新的队长的 ID</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL BroadcastSetLeader(_In_ PGAME_ROOM pRoom, _In_ UINT ID);

/// <summary>
/// 向所有玩家发送队长选择的队员
/// </summary>
/// <param name="pRoom">广播的房间</param>
/// <param name="TeamSize">当前队长选择的队伍人数</param>
/// <param name="TeamArr">当前队长选择的队伍人员的 ID</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL BroadcastSelectTeam(_In_ PGAME_ROOM pRoom, _In_ UINT TeamSize, _In_ UINT32 TeamArr[]);

/// <summary>
/// 向所有玩家发送队长确认了编队
/// </summary>
/// <param name="pRoom">广播的房间</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL BroadcastConfirmTeam(_In_ PGAME_ROOM pRoom);

/// <summary>
/// 向所有玩家发送当前投票进度
/// </summary>
/// <param name="pRoom">广播的房间</param>
/// <param name="VotedCnt">已经投票的人数</param>
/// <param name="VotedIDList">已经投票的玩家 ID</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL BroadcastVoteTeamProgress(_In_ PGAME_ROOM pRoom, _In_ UINT VotedCnt, _In_ UINT32 VotedIDList[]);

/// <summary>
/// 向所有玩家发送投票结果
/// </summary>
/// <param name="pRoom">广播的房间</param>
/// <param name="bVoteResult">投票的结果（通过还是不通过）</param>
/// <param name="VoteListCnt">投票总人数</param>
/// <param name="VoteList">每个人的投票结果</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL BroadcastVoteTeam(_In_ PGAME_ROOM pRoom, _In_ BOOL bVoteResult, _In_ UINT VoteListCnt, _In_ VOTELIST VoteList[]);

/// <summary>
/// 向所有玩家发送任务执行进度
/// </summary>
/// <param name="pRoom">广播的房间</param>
/// <param name="DecidedCnt">做出决定的玩家数量</param>
/// <param name="DecidedIDList">做出决定的玩家</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL BroadcastMissionResultProgress(_In_ PGAME_ROOM pRoom, _In_ UINT DecidedCnt, _In_ UINT32 DecidedIDList[]);

/// <summary>
/// 向所有玩家发送任务执行结果
/// </summary>
/// <param name="pRoom">广播的房间</param>
/// <param name="bMissionSuccess">任务执行成功与否</param>
/// <param name="Perform">执行任务的玩家数量（好票数量）</param>
/// <param name="Screw">破坏任务的玩家数量（坏票数量）</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL BroadcastMissionResult(_In_ PGAME_ROOM pRoom, _In_ BOOL bMissionSuccess, _In_ UINT32 Perform, _In_ UINT32 Screw);

/// <summary>
/// 向所有玩家发送仙女选择验的人
/// </summary>
/// <param name="pRoom">广播的房间</param>
/// <param name="InspectID">仙女选择验的人的 ID</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL BroadcastFairyInspect(_In_ PGAME_ROOM pRoom, _In_ UINT InspectID);

/// <summary>
/// 向所有玩家发送本局游戏结束
/// </summary>
/// <param name="pRoom">广播的房间</param>
/// <param name="bWin">好人方是否胜利</param>
/// <param name="Reason">胜利/失败原因</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL BroadcastEndGame(_In_ PGAME_ROOM pRoom, _In_ BOOL bWin, _In_z_ CHAR Reason[]);

/// <summary>
/// 向所有玩家发送玩家发送的聊天消息
/// </summary>
/// <param name="pRoom">广播的房间</param>
/// <param name="ID">发出聊天消息的玩家的 ID</param>
/// <param name="Message">聊天消息，UTF8 编码</param>
/// <returns>是否遇到致命错误，网络故障不被计入。</returns>
BOOL BroadcastTextMessage(_In_ PGAME_ROOM pRoom, _In_ UINT ID, _In_z_ CHAR Message[]);

#pragma once
#include "common.h"
#include "MessageSender.h"

#define ROOM_NUMBER_MIN 10000
#define ROOM_NUMBER_MAX 99999

#define ROOM_PLAYER_MAX 10
#define ROOM_PLAYER_MIN 5

#define PLAYER_NICK_MAXLEN 32
#define PLAYER_AVATAR_MAXLEN 32
#define ROOM_PASSWORD_MAXLEN 32

// Role definition
#define ROLE_MERLIN   1 // 梅林
#define ROLE_PERCIVAL 2 // 派西维尔
#define ROLE_ASSASSIN 3 // 刺客
#define ROLE_MORDRED  4 // 莫德雷德
#define ROLE_OBERON   5 // 奥伯伦
#define ROLE_MORGANA  6 // 莫甘娜
#define ROLE_LOYALIST 7 // 亚瑟的忠臣
#define ROLE_MINIONS  8 // 莫德雷德的爪牙

#define ENABLE_FAIRY_THRESHOLD 7 // fairy will be enabled when player >= ENABLE_FAIRY_THRESHOLD

// Hint definition
#define HINT_GOOD               1
#define HINT_BAD                2
#define HINT_MERLIN_OR_MORGANA  3
#define HINT_ASSASSIN           4
#define HINT_MORDRED            5
#define HINT_MORGANA            6
#define HINT_MINIONS            7

typedef struct _CONNECTION_INFO CONNECTION_INFO, * PCONNECTION_INFO;

typedef struct _PLAYER_INFO
{
    PCONNECTION_INFO pConnInfo;
    UINT GameID;
    BOOL bIsRoomOwner;
    char NickName[PLAYER_NICK_MAXLEN + 1];
    char Avatar[PLAYER_AVATAR_MAXLEN + 1];
}PLAYER_INFO, *PPLAYER_INFO;

typedef struct _GAME_ROOM
{
    UINT RoomNumber;
    LONG64 RefCnt;

    BOOL bGaming; // is game running. (or waiting otherwise)
    UINT IDCount;

    char Password[ROOM_PASSWORD_MAXLEN + 1];

    SRWLOCK PlayerListLock; // Visiting / Writing following field needs this lock.

    UINT WaitingCount;
    UINT PlayingCount;
    PLAYER_INFO WaitingList[ROOM_PLAYER_MAX]; // Stores only online player info. If a user is offline, will be removed from this list.
    PLAYER_INFO PlayingList[ROOM_PLAYER_MAX]; // Copied from WaitingList when game starts, and not modified until game ends.
                                              //     except pConnInfo field (will be set to NULL if a player is offline)

    UINT RoleList[ROOM_PLAYER_MAX];

    UINT VotedCount;
    VOTELIST VotedIDList[ROOM_PLAYER_MAX];

    UINT Vote[ROOM_PLAYER_MAX];
    UINT LeaderIndex; // current leader
    BOOL bFairyEnabled;
    UINT FairyIndex;

    UINT TeamMemberCnt;
    UINT TeamMemberid[ROOM_PLAYER_MAX];

    UINT DecidedCnt;
    UINT32 DecidedIDList[ROOM_PLAYER_MAX];
    
    UINT Perform;
    UINT Screw;

    UINT Rounds;//游戏轮数

}GAME_ROOM, * PGAME_ROOM;

VOID InitRoomManager(VOID);

BOOL CreateRoom(_Inout_ PCONNECTION_INFO pConnInfo, _In_z_ const char* NickName, _In_opt_z_ const char* Password);

BOOL JoinRoom(_In_ UINT RoomNum, _Inout_ PCONNECTION_INFO pConnInfo, _In_z_ const char* NickName, _In_opt_z_ const char* Password);

VOID LeaveRoom(_Inout_ PCONNECTION_INFO pConnInfo);

BOOL ChangeAvatar(_Inout_ PCONNECTION_INFO pConnInfo, _In_z_ const char* Avatar);

/// <summary>
/// 处理开始游戏
/// </summary>
/// <param name="pConnInfo">发起开始游戏的玩家信息</param>
/// <returns>是否遇到致命错误，网络错误不计。</returns>
BOOL StartGame(_Inout_ PCONNECTION_INFO pConnInfo);

/// <summary>
/// 处理当前队长选择队伍成员
/// </summary>
/// <param name="pConnInfo">玩家信息</param>
/// <param name="TeamMemberCnt">队长选择的人数</param>
/// <param name="TeamMemberList">队长选择的队员 ID</param>
/// <returns>是否遇到致命错误，网络错误不计。</returns>
BOOL PlayerSelectTeam(_Inout_ PCONNECTION_INFO pConnInfo, _In_ UINT TeamMemberCnt, _In_ UINT32 TeamMemberList[]);

/// <summary>
/// 处理队长确认当前队伍成员
/// </summary>
/// <param name="pConnInfo">玩家信息</param>
/// <returns>是否遇到致命错误，网络错误不计。</returns>
BOOL PlayerConfirmTeam(_Inout_ PCONNECTION_INFO pConnInfo);

/// <summary>
/// 处理玩家投票赞成/反对编队成员发起任务
/// </summary>
/// <param name="pConnInfo">玩家信息</param>
/// <param name="bVote">该玩家赞成还是反对</param>
/// <returns>是否遇到致命错误，网络错误不计。</returns>
BOOL PlayerVoteTeam(_Inout_ PCONNECTION_INFO pConnInfo, _In_ BOOL bVote);

/// <summary>
/// 处理玩家投票选择执行还是破坏任务
/// </summary>
/// <param name="pConnInfo">玩家信息</param>
/// <param name="bVote">该玩家选择执行还是破坏</param>
/// <returns>是否遇到致命错误，网络错误不计。</returns>
BOOL PlayerConductMission(_Inout_ PCONNECTION_INFO pConnInfo, _In_ BOOL bPerform);

/// <summary>
/// 处理玩家发起仙女验人
/// </summary>
/// <param name="pConnInfo">玩家信息</param>
/// <param name="ID">玩家选择验的玩家的 ID</param>
/// <returns>是否遇到致命错误，网络错误不计。</returns>
BOOL PlayerFairyInspect(_Inout_ PCONNECTION_INFO pConnInfo, _In_ UINT ID);

/// <summary>
/// 处理刺客发起刺杀
/// </summary>
/// <param name="pConnInfo">玩家信息</param>
/// <param name="ID">玩家选择刺杀的玩家的 ID</param>
/// <returns>是否遇到致命错误，网络错误不计。</returns>
BOOL PlayerAssassinate(_Inout_ PCONNECTION_INFO pConnInfo, _In_ UINT AssassinateID);

/// <summary>
/// 处理玩家发送消息
/// </summary>
/// <param name="pConnInfo">玩家信息</param>
/// <param name="Message">玩家发送的消息，UTF8 编码</param>
/// <returns>是否遇到致命错误，网络错误不计。</returns>
BOOL PlayerTextMessage(_Inout_ PCONNECTION_INFO pConnInfo, _In_z_ const CHAR Message[]);

#ifndef PGZXB_YUNGAMESERVER_FWD_H
#define PGZXB_YUNGAMESERVER_FWD_H

#include <filesystem>
#include <hv/json.hpp>

#include "pg/pgfwd.h"
#include "pg/pgfmt.h"

#define PGYGS_NAMESPACE_START PGZXB_ROOT_NAMESPACE_START namespace ygs {
#define PGYGS_NAMESPACE_END } PGZXB_ROOT_NAMESPACE_END

#ifdef PGYGS_WITH_LOG
#define PGYGS_LOG(fmt, ...) \
    std::cerr << pgfmt::format( \
        "[PGYGS]{0}:{1} LOG: {2}\n", __FILE__, __LINE__, \
        pgfmt::format(fmt, __VA_ARGS__))
#else
#define PGYGS_LOG(...) PGZXB_PASS
#endif

PGYGS_NAMESPACE_START

using ResourceID = int;
using Json = nlohmann::json;

enum class ErrCode : int {
    SUCCESS,
    INTERNAL_ERR,
    FAILED,
    INVALID_PARAM,
    CREATE_OR_ENTER_ROOM_REPEATLY,
    CREATE_ROOM_WITH_EXISTING_ID,
    CREATE_ROOM_WITH_NOT_EXISTING_GAME_ID,
    START_GAME_WITHOUT_CREATING_ROOM,
    PAUSE_GAME_WITHOUT_CREATING_ROOM,
    END_GAME_WITHOUT_CREATING_ROOM,
    START_GAME_BUT_NOT_OWNER,
    PAUSE_GAME_BUT_NOT_OWNER,
    END_GAME_BUT_NOT_OWNER,
    ROOM_NOT_FOUND,
    MEMBER_ID_EXISTING,
};

inline const char *err_code_to_zhCN_str(ErrCode code) {
    static constexpr const char *ZHCN_MSG_TABLE[] = {
        "请求成功",             // SUCCESS,
        "服务器内部错误",        // INTERNAL_ERR
        "请求失败",             // FAILED,
        "非法的请求参数",        // INVALID_PARAM,
        "已加入或创建房间",       // CREATE_OR_ENTER_ROOM_REPEATLY
        "指定的新房间ID已存在",   // CREATE_ROOM_WITH_EXISTING_ID
        "指定创建的游戏不存在",   // CREATE_ROOM_WITH_NOT_EXISTING_GAME_ID
        "开始游戏前请创建房间",   // START_GAME_WITHOUT_CREATING_ROOM
        "还未创建房间",          // PAUSE_GAME_WITHOUT_CREATING_ROOM
        "还未创建房间",          // END_GAME_WITHOUT_CREATING_ROOM
        "只有房主可以开始游戏",   // START_GAME_BUT_NOT_OWNER
        "只有房主可以暂停游戏",   // PAUSE_GAME_BUT_NOT_OWNER
        "只有房主可以结束游戏",   // END_GAME_BUT_NOT_OWNER
        "房间不存在",            // ROOM_NOT_FOUND
        "成员ID已存在",
    };

    return ZHCN_MSG_TABLE[(int)code];
}

PGYGS_NAMESPACE_END
#endif

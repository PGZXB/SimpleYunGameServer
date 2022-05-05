#include "fwd.h"
#include "pg/pgfwd.h"
#include "services.h"
#include "YGSContext.h"
#include <memory>
#include <sstream>
PGYGS_NAMESPACE_START
namespace api::v1 {

struct YGSWSContext { // YunGameServer WebSocket Context
    std::string user_id;
    Room *room{nullptr};
    std::shared_ptr<GameObject> game_obj;
};

using ReqProcFuncArgs = std::vector<std::string>;
using ReqProcFunc = std::function<void(YGSContext *, const WebSocketChannelPtr&, const ReqProcFuncArgs &)>;
using ReqProcFuncTable = ReqProcFunc[26];

static bool registe_request_processing_table(ReqProcFuncTable &table) {
    table['C' - 'A'] = [](YGSContext *ygs_ctx, const WebSocketChannelPtr &channel, const ReqProcFuncArgs &args) {
        if (channel->context()) {
            auto resp_json = make_response_json_data(ErrCode::CREATE_OR_ENTER_ROOM_REPEATLY, nullptr);
            channel->send(resp_json.dump());
            return;
        }
        if (args.size() != 3) {
            auto resp_json = make_response_json_data(ErrCode::INVALID_PARAM, nullptr);
            channel->send(resp_json.dump());
            return;
        }
        const auto &user_id = args[0];
        const auto &room_id = args[1];
        const auto &game_id = args[2]; // Not game-instance id

        auto &room_mgr = ygs_ctx->room_mgr;
        auto &game_creator_mgr = ygs_ctx->game_creator_mgr;
        auto &game_mgr = ygs_ctx->game_mgr;

        // Check room_id
        if (room_mgr.try_lookup_object(room_id, nullptr)) {
            auto resp_json = make_response_json_data(ErrCode::CREATE_ROOM_WITH_EXISTING_ID, nullptr);
            channel->send(resp_json.dump());
            return;
        }
        // Check game_id
        Game::GameCreator *pCreator = nullptr;
        if (!game_creator_mgr.try_lookup_object(game_id, &pCreator)) {
            auto resp_json = make_response_json_data(ErrCode::CREATE_ROOM_WITH_NOT_EXISTING_GAME_ID, nullptr);
            channel->send(resp_json.dump());
            return;
        }
        PGZXB_DEBUG_ASSERT(pCreator && *pCreator);

        auto *ygs_ws_ctx = channel->newContext<YGSWSContext>();
        auto new_game_ins = game_mgr.force_create_object(std::to_string(game_mgr.size()),
            (*pCreator)(std::to_string(game_mgr.size()), game_id));
        auto &new_room = room_mgr.force_create_object(
            room_id, room_id, new_game_ins, user_id, channel);

        ygs_ws_ctx->user_id = user_id;
        ygs_ws_ctx->room = &new_room;
        ygs_ws_ctx->game_obj = new_room.init();

        const auto &res_mgr = new_game_ins->resouce_mgr();
        Json json;
        Json res_arr = Json::array();
        for (const auto &[id, resource] : res_mgr) {
            PGZXB_UNUSED(id);
            res_arr.push_back(resource.to_json());
        }
        json["resources"] = res_arr;
        auto resp_json = make_response_json_data(ErrCode::SUCCESS, json);
        channel->send(resp_json.dump());
    };

#define DEFINE_OP_GAME(opName, errPrefix, opFunc) \
    table[opName - 'A'] = [](YGSContext *ygs_ctx, const WebSocketChannelPtr &channel, const ReqProcFuncArgs &args) { \
        auto *ygs_ws_ctx = channel->getContext<YGSWSContext>(); \
        if (!ygs_ws_ctx) { \
            auto resp_json = make_response_json_data(ErrCode::errPrefix##_GAME_WITHOUT_CREATING_ROOM, nullptr); \
            channel->send(resp_json.dump()); \
            return; \
        } \
        if (ygs_ws_ctx->user_id == ygs_ws_ctx->room->owner_id()) { \
            PGZXB_DEBUG_ASSERT(ygs_ws_ctx->room->owner_wschannel().get() == channel.get()); \
            ygs_ws_ctx->room->game()->opFunc(); \
            auto resp_json = make_response_json_data(ErrCode::SUCCESS, nullptr); \
            channel->send(resp_json.dump()); \
            return; \
        } else { \
            auto resp_json = make_response_json_data(ErrCode::errPrefix##_GAME_BUT_NOT_OWNER, nullptr); \
            channel->send(resp_json.dump()); \
            return; \
        } \
    }

    DEFINE_OP_GAME('S', START, start);
    DEFINE_OP_GAME('P', PAUSE, pause);
    DEFINE_OP_GAME('E', END, stop);
#undef DEFINE_OP_GAME

    table['I' - 'A'] = [](YGSContext *ygs_ctx, const WebSocketChannelPtr &channel, const ReqProcFuncArgs &args) {
        auto *ygs_ws_ctx = channel->getContext<YGSWSContext>();
        if (!ygs_ws_ctx) {
            auto resp_json = make_response_json_data(ErrCode::ROOM_NOT_FOUND, nullptr);
            channel->send(resp_json.dump());
            return;
        }
        
        auto &go = ygs_ws_ctx->game_obj;
        for (const auto &e : args) {
            go->push_event((Event::Event)atoi(e.c_str()));
        }
    };

    table['R' - 'A'] = [](YGSContext *ygs_ctx, const WebSocketChannelPtr &channel, const ReqProcFuncArgs &args) {
        if (channel->context()) {
            auto resp_json = make_response_json_data(ErrCode::CREATE_OR_ENTER_ROOM_REPEATLY, nullptr);
            channel->send(resp_json.dump());
            return;
        }
        if (args.size() != 2) {
            auto resp_json = make_response_json_data(ErrCode::INVALID_PARAM, nullptr);
            channel->send(resp_json.dump());
            return;
        }

        const auto &user_id = args[0];
        const auto &room_id = args[1];
        auto &room_mgr = ygs_ctx->room_mgr;

        // Check room_id
        Room *pRoom = nullptr;
        if (!room_mgr.try_lookup_object(room_id, &pRoom)) {
            auto resp_json = make_response_json_data(ErrCode::ROOM_NOT_FOUND, nullptr);
            channel->send(resp_json.dump());
            return;
        }
        PGZXB_DEBUG_ASSERT(pRoom);

        std::shared_ptr<GameObject> pGO{nullptr};
        if (!pRoom->try_add_member(user_id, channel, &pGO)) {
            auto resp_json = make_response_json_data(ErrCode::MEMBER_ID_EXISTING, nullptr);
            channel->send(resp_json.dump());
            return;
        }

        auto *ygs_ws_ctx = channel->newContext<YGSWSContext>();
        ygs_ws_ctx->user_id = user_id;
        ygs_ws_ctx->room = pRoom;
        ygs_ws_ctx->game_obj = std::move(pGO);

        const auto &res_mgr = pRoom->game()->resouce_mgr();
        Json json = Json::object(), res_arr = Json::array();
        for (const auto &[id, resource] : res_mgr) {
            PGZXB_UNUSED(id);
            res_arr.push_back(resource.to_json());
        }
        json["resources"] = res_arr;
        auto resp_json = make_response_json_data(ErrCode::SUCCESS, json);
        channel->send(resp_json.dump());
    };

    return std::rand() % 2;
}

static void process_request(YGSContext *ygs_ctx, const WebSocketChannelPtr& channel, const std::string& msg) {
    static ReqProcFunc REQUEST_PROCESSING_TABLE[('Z' - 'A') + 1];
    [[maybe_unused]] static bool flag = registe_request_processing_table(REQUEST_PROCESSING_TABLE);
    
    std::string op;
    std::vector<std::string> args;

    std::istringstream iss(msg);
    iss >> op;
    if (op.size() != 1 && (op.front() < 'A' || op.front() > 'Z')) {
        auto resp_json = make_response_json_data(ErrCode::INVALID_PARAM, nullptr);
        channel->send(resp_json.dump());
        return;
    }

    while (true) {
        std::string temp;
        iss >> temp;
        if (temp.empty()) break;
        args.push_back(temp);
    }

    if (auto func = REQUEST_PROCESSING_TABLE[op.front() - 'A']) {
        func(ygs_ctx, channel, args);
    } else {
        auto resp_json = make_response_json_data(ErrCode::INVALID_PARAM, nullptr);
        channel->send(resp_json.dump());
    }
}

hv::WebSocketService new_websocket_service(YGSContext *ygs_ctx) {
    WebSocketService ws;
    ws.onopen = [](const WebSocketChannelPtr& channel, const std::string& url) {
        PGYGS_LOG("New WebSocket connection(from=\"{0}\", with url=\"{1}\")",
            channel->peeraddr(), url);
        channel->setContext(nullptr);
    };

    ws.onmessage = [ygs_ctx](const WebSocketChannelPtr& channel, const std::string& msg) {
        // Create room: "C User-ID Room-ID Game-ID", Response-data: [{"id":, "url":},...] (resources)
        // Start game:  "S",                         Response-data: null
        // Pause game:  "P",                         Response-data: null
        // End game:    "E",                         Response-data: null
        // enter Room:  "R User-ID Room-ID",         Response-data: [{"id":, "url":},...] (resources)
        // Input:       "I keycode",                 Don't responce
        process_request(ygs_ctx, channel, msg);
    };

    ws.onclose = [ygs_ctx](const WebSocketChannelPtr& channel) {
        PGYGS_LOG("Close WebSocket connection(from=\"{0}\")", channel->peeraddr());
        if (channel->context()) {
            auto *ygs_ws_ctx = channel->getContext<YGSWSContext>();
            if (ygs_ws_ctx->user_id == ygs_ws_ctx->room->owner_id()) {
                auto game = ygs_ws_ctx->room->game();
                auto game_id = game->id();
                auto human_friendly_game_id = game_id + ":" + game->id();

                game->force_stop();
                game->wait_game_loop();

                PGYGS_LOG("Ref count of game={0}: {1}", human_friendly_game_id, game.use_count());
                auto removed = ygs_ctx->room_mgr.try_remove_object(ygs_ws_ctx->room->id());
                PGZXB_DEBUG_ASSERT(removed);
                removed = ygs_ctx->game_mgr.try_remove_object(game_id);
                PGZXB_DEBUG_ASSERT(removed);
                PGYGS_LOG("Ref count of game={0}: {1}", human_friendly_game_id, game.use_count());
                PGZXB_DEBUG_ASSERT(game.use_count() == 1);
            } else {
                PGZXB_DEBUG_ASSERT(ygs_ws_ctx->game_obj);
                PGYGS_LOG("User(user_id={0}, game_obj={1}) exiting", ygs_ws_ctx->user_id, ygs_ws_ctx->game_obj->id());
                ygs_ws_ctx->room->try_remove_member(ygs_ws_ctx->user_id);
                ygs_ws_ctx->game_obj->push_event(Event::GO_DEAD);
            }
            channel->deleteContext<YGSWSContext>();
        }
    };
    return ws;
}

}
PGYGS_NAMESPACE_END

#include "services.h"

#include <hv/HttpService.h>
#include <hv/WebSocketServer.h>
#include <hv/httpdef.h>
#include "utils.h"

PGYGS_NAMESPACE_START
namespace api::v1 {

http_ctx_handler new_GET_get_rooms_router(const Room::Mgr *room_mgr) {
    return [room_mgr](const HttpContextPtr& ctx) -> int {
        if (room_mgr == nullptr) {
            ctx->sendJson(make_response_json_data(ErrCode::INTERNAL_ERR, nullptr));
            return HTTP_STATUS_INTERNAL_SERVER_ERROR;
        }

        Json rooms = Json::array(); // I know the 'room' is uncountable noun
        for (const auto &[id, room] : *room_mgr) {
            PGZXB_UNUSED(id);
            rooms.push_back(room.to_json());
        }

        ctx->sendJson(make_response_json_data(ErrCode::SUCCESS, rooms));
        return HTTP_STATUS_OK;
    };
}

http_ctx_handler new_GET_get_games_router(const Game::GameCreatorMgr *game_creator_mgr) {
    return [game_creator_mgr](const HttpContextPtr& ctx) -> int {
        if (game_creator_mgr == nullptr) {
            ctx->sendJson(make_response_json_data(ErrCode::INTERNAL_ERR, nullptr));
            return HTTP_STATUS_INTERNAL_SERVER_ERROR;
        }

        Json games = Json::array();
        for (const auto &[id, game_creator] : *game_creator_mgr) {
            games.push_back(id);
            PGZXB_UNUSED(game_creator);
        }

        ctx->sendJson(make_response_json_data(ErrCode::SUCCESS, games));
        return HTTP_STATUS_OK;
    };
}

}
PGYGS_NAMESPACE_END

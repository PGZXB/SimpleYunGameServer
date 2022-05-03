#ifndef PGZXB_YUNGAMESERVER_RUNSERVICE_H
#define PGZXB_YUNGAMESERVER_RUNSERVICE_H

#include <hv/HttpContext.h>
#include <hv/HttpService.h>
#include <hv/WebSocketServer.h>

#include "fwd.h"
#include "Room.h"
#include "Game.h"
PGYGS_NAMESPACE_START
struct YGSContext;

namespace api::v1 {

http_ctx_handler new_GET_get_rooms_router(const Room::Mgr *room_mgr);
http_ctx_handler new_GET_get_games_router(const Game::GameCreatorMgr *game_creator_mgr);

hv::WebSocketService new_websocket_service(YGSContext *ygs_ctx);

}
PGYGS_NAMESPACE_END
#endif

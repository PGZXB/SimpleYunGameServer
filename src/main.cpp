#include "fwd.h"
#include "games.h"
#include "YGSContext.h"
#include "api/v1/services.h"

#undef PGZXB_DEBUG_INFO_HEADER
#define PGZXB_DEBUG_INFO_HEADER "[YunGameServer-MAIN-LOG] "

using namespace pg::ygs;

int main() {
    PGZXB_DEBUG_Print("Starting YunGameServer ---");

    hv::HttpService http;
    hv::WebSocketService ws;

    YGSContext ygs_ctx;
    ygs_ctx.game_creator_mgr.force_create_object("tank_game", examples::create_tank_game);

    http.GET("/api/v1/rooms", api::v1::new_GET_get_rooms_router(&ygs_ctx.room_mgr));
    http.GET("/api/v1/games", api::v1::new_GET_get_games_router(&ygs_ctx.game_creator_mgr));
    ws = api::v1::new_websocket_service(&ygs_ctx);

    websocket_server_t server;
    server.port = 8899;
    server.service = &http;
    server.ws = &ws;
    websocket_server_run(&server, 0);

    // press Enter to stop
    while (getchar() != '\n');
    websocket_server_stop(&server);

    // Create room: "C User-ID Room-ID Game-ID", Response-data: [{"id":, "url":},...] (resources)
    // Start game:  "S",                         Response-data: null
    // Pause game:  "P",                         Response-data: null
    // End game:    "E",                         Response-data: null
    // enter Room:  "R User-ID Room-ID",         Response-data: [{"id":, "url":},...] (resources)
    

    PGZXB_DEBUG_Print("Exiting YunGameServer  ---");
    return 0;
}

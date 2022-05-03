#include "fwd.h"
#include "Room.h"
#include "Game.h"
PGYGS_NAMESPACE_START

struct YGSContext {
    Room::Mgr room_mgr;
    Game::GameCreatorMgr game_creator_mgr;
    Game::GameMgr game_mgr;
};

PGYGS_NAMESPACE_END

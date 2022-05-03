#ifndef PGZXB_YUNGAMESERVER_GAMES_H
#define PGZXB_YUNGAMESERVER_GAMES_H

#include "fwd.h"
#include <memory>
PGYGS_NAMESPACE_START
class Game;

namespace examples {

std::shared_ptr<Game> create_tank_game(const std::string &, const std::string &);

}
PGYGS_NAMESPACE_END
#endif

#ifndef PGZXB_YUNGAMESERVER_GAMEOBJECTCONTROLLER_H
#define PGZXB_YUNGAMESERVER_GAMEOBJECTCONTROLLER_H

#include "fwd.h"
#include "utils.h"
#include "GameObject.h"
#include <cstdint>
#include <functional>
#include <memory>
PGYGS_NAMESPACE_START

class Game;
class GameObject;
using GameObjectTickTask = std::function<
    void(std::shared_ptr<Game>, std::shared_ptr<GameObject>, std::uint64_t)>;

class GameObjectController {
public:
    const GameObjectTickTask &tick_task() const {
        return tick_task_;
    }

    GameObjectTickTask &tick_task() {
        return tick_task_;
    }

    void set_tick_task(const GameObjectTickTask &task) {
        tick_task_ = [task](std::shared_ptr<Game> game, std::shared_ptr<GameObject> go, std::uint64_t events) {
            if (events & Event::GO_DEAD) {
                go->will_dead();
            }
            if (task) task(std::move(game), std::move(go), events);
        };
    }
private:
    GameObjectTickTask tick_task_{nullptr};
};

PGYGS_NAMESPACE_END
#endif

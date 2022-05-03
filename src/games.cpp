#include "games.h"
#include "Game.h"
#include "Resource.h"
#include <memory>
PGYGS_NAMESPACE_START
namespace examples {

std::shared_ptr<Game> create_tank_game(const std::string &id, const std::string &game_id) {
    auto tank_game = std::make_shared<Game>(id, game_id);

    Resource::Mgr res_mgr;
    res_mgr.force_create_object(0, 0, get_assets_dir() + "tankgame_background.gif");
    res_mgr.force_create_object(1, 1, get_assets_dir() + "tankgame_player_tank.gif");
    res_mgr.force_create_object(2, 2, get_assets_dir() + "tankgame_npc_tank.gif");
    res_mgr.force_create_object(3, 3, get_assets_dir() + "tankgame_wall.gif");
    res_mgr.force_create_object(4, 4, get_assets_dir() + "tankgame_bullet.gif");

    std::shared_ptr<GameObjectController> player_tank_controller = std::make_shared<GameObjectController>();
    player_tank_controller->set_tick_task(
        [](std::shared_ptr<Game> game, std::shared_ptr<GameObject> go, std::uint64_t events) {
            auto old_thera = go->obb().theta_;
            go->obb().theta_ = 0;
            go->velocity() = {0, 0};

            if (events & Event::KEY_PRESS_W) {
                go->obb().theta_ += 270;
                go->velocity() += Vec2<int>{0, -5};
            } else if (events & Event::KEY_PRESS_S) {
                go->obb().theta_ += 90;
                go->velocity() += Vec2<int>{0, 5};
            } else if (events & Event::KEY_PRESS_A) {
                go->obb().theta_ += 180;
                go->velocity() += Vec2<int>{-5, 0};
            } else if (events & Event::KEY_PRESS_D) {
                go->obb().theta_ += 0;
                go->velocity() += Vec2<int>{5, 0};
            }

            if (go->velocity().x == 0 && go->velocity().y == 0) {
                go->obb().theta_ = old_thera;
            }

            go->obb().area_.top_left += go->velocity();
        });

    Game::GameObjectCreatorMgr go_creator_mgr;
    go_creator_mgr.force_create_object("player", [player_tank_controller]() {
        auto tank = std::make_shared<GameObject>();
        tank->set_surface_img_id(1)
            .set_velocity(Vec2<int>{0, 0})
            .set_obb(OBB<int>{Rect<int>{Point<int>{0, 0}, 50, 50}, 270})
            .set_controller(player_tank_controller);
        return tank;
    });
    tank_game->set_game_object_creator_mgr(go_creator_mgr);

    return tank_game;
}

}
PGYGS_NAMESPACE_END

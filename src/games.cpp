#include "games.h"
#include "Game.h"
#include "Resource.h"
#include <cstddef>
#include <memory>
#include <random>
PGYGS_NAMESPACE_START
namespace examples {

std::shared_ptr<Game> create_tank_game(const std::string &id, const std::string &game_id) {
    constexpr const std::size_t TANK_WIDTH = 50;
    constexpr const std::size_t TANK_HEIGHT = 50;
    constexpr const std::size_t TANK_SPEED = 5;
    constexpr const std::size_t BULLET_WIDTH_HEIGHT = 5;
    constexpr const std::size_t BULLET_SPEED = 20;
    constexpr const std::size_t BACKGROUND_WIDTH = 800;
    constexpr const std::size_t BACKGROUND_HEIGHT = 600;
    constexpr const std::size_t DEFAULT_NPC_CNT = 3;
    constexpr const ResourceID BACKGROUND_RES_ID = 0;
    constexpr const ResourceID PLAYER_TANK_RES_ID = 1;
    constexpr const ResourceID PLAYER_BULLET_RES_ID = 2;
    constexpr const ResourceID NPC_TANK_RES_ID = 3;
    constexpr const ResourceID NPC_BULLET_RES_ID = 4;

    enum class GOType : int {
        PLAYER_TANK,
        NPC_TANK,
        BULLET,
        BACKGROUND,
        WALL,
    };
    
    auto tank_game = std::make_shared<Game>(id, game_id);

    Resource::Mgr res_mgr;
    res_mgr.force_create_object(BACKGROUND_RES_ID,    BACKGROUND_RES_ID,    "tankgame_background.jpg"        );
    res_mgr.force_create_object(PLAYER_TANK_RES_ID,   PLAYER_TANK_RES_ID,   "tankgame_player_tank.gif"       );
    res_mgr.force_create_object(PLAYER_BULLET_RES_ID, PLAYER_BULLET_RES_ID, "tankgame_player_tank_bullet.gif");
    res_mgr.force_create_object(NPC_TANK_RES_ID,      NPC_TANK_RES_ID,      "tankgame_npc_tank.gif"          );
    res_mgr.force_create_object(NPC_BULLET_RES_ID,    NPC_BULLET_RES_ID,    "tankgame_npc_tank_bullet.gif"   );

    std::shared_ptr<GameObjectController> player_tank_controller = std::make_shared<GameObjectController>();
    std::shared_ptr<GameObjectController> bullet_controller = std::make_shared<GameObjectController>();
    
    struct NPCTankTickTask {
        mutable int dest_tick_count{0};
        mutable int current_tick_count{0};

        void operator() (std::shared_ptr<Game> game, std::shared_ptr<GameObject> go, std::uint64_t events) {
            if (current_tick_count == dest_tick_count) {
                dest_tick_count = std::rand() % (FPS * 5) + FPS * 3;
                current_tick_count = 0;
                go->aabb().theta_ = (std::rand() % 4) * 90;
                auto unit_vec = axis_theta_to_unit_vec2<int>(go->aabb().theta_);
                go->velocity() = (unit_vec *= TANK_SPEED);
            }

            if (events & Event::COLLISION) {
                auto &collision_with = *go->context<std::vector<std::shared_ptr<GameObject>>>();
                for (const auto & e : collision_with) {
                    if (e->type() == (int)GOType::WALL) {
                        auto dir_unit_vec = axis_theta_to_unit_vec2<int>(go->aabb().theta_);
                        go->velocity() = (dir_unit_vec *= -TANK_SPEED);
                        go->aabb().theta_ += 180; // Reverse direction
                        if ((int)go->aabb().theta_ >= 360) go->aabb().theta_ -= 360;
                    } else if (e->type() == (int)GOType::BULLET) {
                        go->will_dead();
                    } else if (e->type() == (int)GOType::BACKGROUND) {
                        if (!go->aabb().to_rect().inner(e->aabb().to_rect())) {
                            auto dir_unit_vec = axis_theta_to_unit_vec2<int>(go->aabb().theta_);
                            go->velocity() = (dir_unit_vec *= -TANK_SPEED);
                            go->aabb().theta_ += 180; // Reverse direction
                            if ((int)go->aabb().theta_ >= 360) go->aabb().theta_ -= 360;
                        }
                    }
                }
            }            

            ++current_tick_count;
            go->aabb().area_.top_left += go->velocity();
        };
    };

    bullet_controller->set_tick_task(
        [](std::shared_ptr<Game> game, std::shared_ptr<GameObject> go, std::uint64_t events) {
            go->aabb().area_.top_left += go->velocity();
            // if (!go->aabb().intersect(game->background().aabb())) {
            //     // Go out => Die
            //     go->will_dead();
            // }
            if (events & Event::COLLISION) {
                auto &collision_with = *go->context<std::vector<std::shared_ptr<GameObject>>>();
                for (const auto & e : collision_with) {
                    if (e->type() == (int)GOType::WALL) {
                        go->will_dead();
                        break;
                    } else if (e->type() == (int)GOType::BULLET) {
                        // Pass
                    } else if (e->type() == (int)GOType::BACKGROUND) {
                        if (!go->aabb().to_rect().inner(e->aabb().to_rect())) {
                            go->will_dead();
                        }
                    }
                }
            }
        }
    );

    player_tank_controller->set_tick_task(
        [](std::shared_ptr<Game> game, std::shared_ptr<GameObject> go, std::uint64_t events) {
            go->velocity() = {0, 0};

            if (events & Event::COLLISION) {
                auto &collision_with = *go->context<std::vector<std::shared_ptr<GameObject>>>();
                for (const auto & e : collision_with) {
                    if (e->type() == (int)GOType::WALL) {
                        auto dir_unit_vec = axis_theta_to_unit_vec2<int>(go->aabb().theta_);
                        go->aabb().area_.top_left -= (dir_unit_vec *= TANK_SPEED);
                    } else if (e->type() == (int)GOType::BULLET) {
                        go->will_dead();
                    } else if (e->type() == (int)GOType::BACKGROUND) {
                        if (!go->aabb().to_rect().inner(e->aabb().to_rect())) {
                            auto dir_unit_vec = axis_theta_to_unit_vec2<int>(go->aabb().theta_);
                            go->aabb().area_.top_left -= (dir_unit_vec *= TANK_SPEED);
                        }
                    }
                }
            }

            if (events & Event::KEY_PRESS_SPACE) {
                game->run_in_game_loop([game, go]() {
                    std::function<std::shared_ptr<GameObject>()> *pCreator{nullptr};
                    auto found = game->game_object_creator_mgr().try_lookup_object("bullet", &pCreator);
                    PGZXB_DEBUG_ASSERT(found);
                    PGZXB_DEBUG_ASSERT(pCreator && *pCreator);
                    auto bullet = (*pCreator)();

                    auto unit_vec = axis_theta_to_unit_vec2<int>(go->aabb().theta_);
                    auto offset = unit_vec;
                    offset *= (go->aabb().area_.height / 2);
                    offset += (axis_theta_to_unit_vec2<int>(go->aabb().theta_) *= 16);
                    auto fire_point = (go->aabb().area_.center() += offset);
                    bullet->set_velocity(unit_vec *= BULLET_SPEED)
                        .set_aabb(
                            AABB<int>{ // bounding box
                                Rect<int>{
                                    fire_point,
                                    BULLET_WIDTH_HEIGHT,
                                    BULLET_WIDTH_HEIGHT
                                },
                                270 // theta
                            }
                        );
                    game->add_game_object(bullet);
                });
            }

            if (events & Event::KEY_PRESS_W) {
                go->aabb().theta_ = 270;
                go->velocity() = Vec2<int>{0, -5};
            } else if (events & Event::KEY_PRESS_S) {
                go->aabb().theta_ = 90;
                go->velocity() = Vec2<int>{0, 5};
            } else if (events & Event::KEY_PRESS_A) {
                go->aabb().theta_ = 180;
                go->velocity() = Vec2<int>{-5, 0};
            } else if (events & Event::KEY_PRESS_D) {
                go->aabb().theta_ = 0;
                go->velocity() = Vec2<int>{5, 0};
            }

            go->aabb().area_.top_left += go->velocity();
        }
    );

    Game::GameObjectCreatorMgr go_creator_mgr;
    go_creator_mgr.force_create_object("player", [player_tank_controller]() {
        auto tank = std::make_shared<GameObject>();
        tank->set_type(GOType::PLAYER_TANK)
            .set_surface_img_id(PLAYER_TANK_RES_ID)
            .set_velocity(Vec2<int>{0, 0})
            .set_aabb(AABB<int>{Rect<int>{Point<int>{0, 0}, TANK_WIDTH, TANK_HEIGHT}, 270})
            .set_controller(player_tank_controller);
        return tank;
    });
    go_creator_mgr.force_create_object("bullet", [bullet_controller]() {
        auto bullet = std::make_shared<GameObject>();
        bullet->set_type(GOType::BULLET)
            .set_surface_img_id(PLAYER_BULLET_RES_ID)
            .set_controller(bullet_controller);
        return bullet;
    });
    go_creator_mgr.force_create_object("npc-tank", [tank_game]() {
        auto npc_tank_controller = std::make_shared<GameObjectController>();
        npc_tank_controller->set_tick_task(NPCTankTickTask());
        auto npc = std::make_shared<GameObject>();
        auto randx = std::rand() % tank_game->background().aabb().area_.center().x;
        npc->set_type(GOType::NPC_TANK)
            .set_aabb(AABB<int>{Rect<int>{Point<int>{randx, 0}, TANK_WIDTH, TANK_HEIGHT}, 270})
            .set_velocity(Vec2<int>{0, 0})
            .set_surface_img_id(NPC_TANK_RES_ID)
            .set_controller(npc_tank_controller);
        return npc;
    });

    GameObject background_object;
    background_object.set_type(GOType::BACKGROUND)
        .set_controller(nullptr)
        .set_surface_img_id(BACKGROUND_RES_ID)
        .set_velocity({0, 0})
        .set_aabb(
            AABB<int>{ // bounding box
                Rect<int>{
                    {0, 0},
                    BACKGROUND_WIDTH,
                    BACKGROUND_HEIGHT
                }, 
                270 // theta
            }
        );

    // init tank-game
    tank_game->set_resources(res_mgr);
    tank_game->set_background(std::move(background_object));
    tank_game->set_game_object_creator_mgr(go_creator_mgr);

    for (std::size_t i = 0; i < DEFAULT_NPC_CNT; ++i) {
        tank_game->run_in_game_loop([tank_game]() {
            std::function<std::shared_ptr<GameObject>()> *pCreator{nullptr};
            auto found = tank_game->game_object_creator_mgr().try_lookup_object("npc-tank", &pCreator);
            PGZXB_DEBUG_ASSERT(found);
            PGZXB_DEBUG_ASSERT(pCreator && *pCreator);
            tank_game->add_game_object((*pCreator)());
        });
    }

    return tank_game;
}

}
PGYGS_NAMESPACE_END

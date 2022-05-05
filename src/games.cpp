#include "games.h"
#include "Game.h"
#include "RenderOrder.h"
#include "Resource.h"
#include "GOs/gos.h"
#include "fwd.h"
#include <memory>
#include <random>
PGYGS_NAMESPACE_START
namespace examples {

std::shared_ptr<Game> create_tank_game(const std::string &id, const std::string &game_id) {
    constexpr const std::size_t TANK_WIDTH = 50;
    constexpr const std::size_t TANK_HEIGHT = 50;
    constexpr const std::size_t PLAYER_TANK_SPEED = 5;
    constexpr const std::size_t NPC_TANK_SPEED = 2;
    constexpr const std::size_t BULLET_WIDTH_HEIGHT = 5;
    constexpr const std::size_t BULLET_SPEED = 20;
    constexpr const std::size_t A_WALL_WIDTH_HEIGHT = 50;
    constexpr const std::size_t BACKGROUND_WIDTH = 800;
    constexpr const std::size_t BACKGROUND_HEIGHT = 600;
    constexpr const std::size_t DEFAULT_NPC_CNT = 3;
    constexpr const ResourceID BACKGROUND_RES_ID = 0;
    constexpr const ResourceID PLAYER_TANK_RES_ID = 1;
    constexpr const ResourceID PLAYER_BULLET_RES_ID = 2;
    constexpr const ResourceID NPC_TANK_RES_ID = 3;
    constexpr const ResourceID NPC_BULLET_RES_ID = 4;
    constexpr const ResourceID WALL_RES_ID = 5;

    enum class GOType : int {
        PLAYER_TANK = (int)gos::GOType::GOTYPE_COUNT,
        NPC_TANK,
        BULLET,
        BACKGROUND,
        WALL,
    };

#ifdef PGYGS_WITH_LOG
    auto tank_game = std::shared_ptr<Game>(
        new Game(id, game_id), 
        [](Game *ptr){
            PGYGS_LOG("Deleting Game {0}", ptr->id());
            delete ptr;
        }
    );
#else
    auto tank_game = std::make_shared<Game>(id, game_id);
#endif

    Resource::Mgr res_mgr;
    res_mgr.force_create_object(BACKGROUND_RES_ID,    BACKGROUND_RES_ID,    "tankgame_background.jpg"        );
    res_mgr.force_create_object(PLAYER_TANK_RES_ID,   PLAYER_TANK_RES_ID,   "tankgame_player_tank.gif"       );
    res_mgr.force_create_object(PLAYER_BULLET_RES_ID, PLAYER_BULLET_RES_ID, "tankgame_player_tank_bullet.gif");
    res_mgr.force_create_object(NPC_TANK_RES_ID,      NPC_TANK_RES_ID,      "tankgame_npc_tank.gif"          );
    res_mgr.force_create_object(NPC_BULLET_RES_ID,    NPC_BULLET_RES_ID,    "tankgame_npc_tank_bullet.gif"   );
    res_mgr.force_create_object(WALL_RES_ID,          WALL_RES_ID,          "tankgame_wall.gif"              );
    
    std::vector<ResourceID> tank_blast_animation_imgs = {6, 7, 8, 9, 10, 11, 12, 13};
    for (auto id : tank_blast_animation_imgs) {
        res_mgr.force_create_object(id, id, "tankgame_blast" + std::to_string(id - 6 + 1) + ".gif");
    }

    std::shared_ptr<GameObjectController> player_tank_controller = std::make_shared<GameObjectController>();
    std::shared_ptr<GameObjectController> bullet_controller = std::make_shared<GameObjectController>();
    
    // struct 

    struct NPCTankTickTask {
        mutable int dest_tick_count{0};
        mutable int current_tick_count{0};
        std::vector<ResourceID> tank_blast_animation_imgs;

        void operator() (std::shared_ptr<Game> game, std::shared_ptr<GameObject> go, std::uint64_t events) {
            if (current_tick_count == dest_tick_count) {
                dest_tick_count = std::rand() % (FPS * 5) + FPS * 3;
                current_tick_count = 0;
                go->aabb().theta_ = (std::rand() % 4) * 90;
                auto unit_vec = axis_theta_to_unit_vec2<int>(go->aabb().theta_);
                go->velocity() = (unit_vec *= NPC_TANK_SPEED);
            }

            if (events & Event::COLLISION) {
                auto &collision_with = *go->context<std::vector<std::shared_ptr<GameObject>>>();
                for (const auto & e : collision_with) {
                    if (e->type() == (int)GOType::WALL) {
                        auto dir_unit_vec = axis_theta_to_unit_vec2<int>(go->aabb().theta_);
                        go->velocity() = (dir_unit_vec *= -NPC_TANK_SPEED);
                        go->aabb().theta_ += 180; // Reverse direction
                        if ((int)go->aabb().theta_ >= 360) go->aabb().theta_ -= 360;
                    } else if (e->type() == (int)GOType::BULLET) {
                        go->will_dead();
                        game->run_in_game_loop([game, go, this]() {
                            using namespace std::chrono_literals;
                            game->add_game_object(
                                gos::create_animation(
                                    go->aabb(), tank_blast_animation_imgs, 100ms, false)
                            );
                        });
                    } else if (e->type() == (int)GOType::BACKGROUND) {
                        if (!go->aabb().to_rect().inner(e->aabb().to_rect())) {
                            auto dir_unit_vec = axis_theta_to_unit_vec2<int>(go->aabb().theta_);
                            go->velocity() = (dir_unit_vec *= -NPC_TANK_SPEED);
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
        [tank_blast_animation_imgs](std::shared_ptr<Game> game, std::shared_ptr<GameObject> go, std::uint64_t events) {
            go->velocity() = {0, 0};

            if (events & Event::COLLISION) {
                auto &collision_with = *go->context<std::vector<std::shared_ptr<GameObject>>>();
                for (const auto & e : collision_with) {
                    if (e->type() == (int)GOType::WALL) {
                        auto dir_unit_vec = axis_theta_to_unit_vec2<int>(go->aabb().theta_);
                        go->aabb().area_.top_left -= (dir_unit_vec *= PLAYER_TANK_SPEED);
                    } else if (e->type() == (int)GOType::BULLET) {
                        go->will_dead();
                        game->run_in_game_loop([game, go, tank_blast_animation_imgs]() {
                            using namespace std::chrono_literals;
                            game->add_game_object(
                                gos::create_animation(
                                    go->aabb(), tank_blast_animation_imgs, 1ms, false)
                            );
                        });
                    } else if (e->type() == (int)GOType::BACKGROUND) {
                        if (!go->aabb().to_rect().inner(e->aabb().to_rect())) {
                            auto dir_unit_vec = axis_theta_to_unit_vec2<int>(go->aabb().theta_);
                            go->aabb().area_.top_left -= (dir_unit_vec *= PLAYER_TANK_SPEED);
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
    go_creator_mgr.force_create_object("npc-tank", [tank_game, tank_blast_animation_imgs]() {
        auto npc_tank_controller = std::make_shared<GameObjectController>();
        NPCTankTickTask tick_task;
        tick_task.tank_blast_animation_imgs = tank_blast_animation_imgs;
        npc_tank_controller->set_tick_task(tick_task);
        auto npc = std::make_shared<GameObject>();
        auto randx = std::rand() % tank_game->background().aabb().area_.center().x;
        npc->set_type(GOType::NPC_TANK)
            .set_aabb(AABB<int>{Rect<int>{Point<int>{randx, 0}, TANK_WIDTH, TANK_HEIGHT}, 270})
            .set_velocity(Vec2<int>{0, 0})
            .set_surface_img_id(NPC_TANK_RES_ID)
            .set_controller(npc_tank_controller);
        return npc;
    });
    go_creator_mgr.force_create_object("wall", []() {
        auto wall = std::make_shared<GameObject>();
        auto gen_wall_render_orders = [](GameObject& obj) {
            // PGZXB_DEBUG_ASSERT((int)obj.aabb().theta_ == 270);
            const auto [top_left, width, height] = obj.aabb().area_;
            const auto [startx, starty] = top_left;
            const auto rows = height / A_WALL_WIDTH_HEIGHT;
            const auto cols = width  / A_WALL_WIDTH_HEIGHT;

            std::vector<RenderOrder> orders;
            for (std::size_t i = 0; i < cols; ++i) {
                for (std::size_t j = 0; j < rows; ++j) {
                    RenderOrder order;
                    order.code = RenderOrder::Code::DRAW;
                    order.args = {
                        WALL_RES_ID,
                        (int)(startx + i * A_WALL_WIDTH_HEIGHT),
                        (int)(starty + j * A_WALL_WIDTH_HEIGHT),
                        A_WALL_WIDTH_HEIGHT,
                        A_WALL_WIDTH_HEIGHT,
                        (int)(obj.aabb().theta_ * DOUBLE2INT_FACTOR),
                    };
                    orders.push_back(order);
                }
            }

            return orders;
        };
        wall->set_type(GOType::WALL)
            .set_surface_img_id(WALL_RES_ID)
            .set_velocity(Vec2<int>{0, 0})
            .set_aabb(AABB<int>{Rect<int>{Point<int>{0, 0}, A_WALL_WIDTH_HEIGHT, A_WALL_WIDTH_HEIGHT}, 270})
            .set_controller(nullptr)
            .set_gen_rendering_orders_callback(gen_wall_render_orders);
        return wall;
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

    // Gen Wall
    auto gen_wall = [tank_game](int x, int y, int m, int n) { // (x, y), mxn
        tank_game->run_in_game_loop([tank_game, x, y, m, n]() {
            std::function<std::shared_ptr<GameObject>()> *pCreator{nullptr};
            auto found = tank_game->game_object_creator_mgr().try_lookup_object("wall", &pCreator);
            PGZXB_DEBUG_ASSERT(found);
            PGZXB_DEBUG_ASSERT(pCreator && *pCreator);
            auto wall = (*pCreator)();
            wall->set_aabb(
                AABB<int>{ // bounding box
                    Rect<int>{
                        {x, y},
                        (int)(n * A_WALL_WIDTH_HEIGHT), // width
                        (int)(m * A_WALL_WIDTH_HEIGHT), // height
                    }, 
                    0 // theta
                }
            );
            tank_game->add_game_object(wall);
        });
    };
    // ┌────────────────────────────────────────────────────────────────────────────────┐
    // │                                                                                │
    // │                                                                                │
    // │        ┌───────────┐               ┌─────┐                                     │
    // │        │           │               │     │                                     │
    // │        │           │               │     │                                     │
    // │        │    ┌──────┘               │     │         ┌─────────────────┐         │
    // │        │    │                      │     │         │                 │         │
    // │        │    │                      │     │         │                 │         │
    // │        │    │                      │     │         └───────────┐     │         │
    // │        │    │                      │     │                     │     │         │
    // │        │    │                      │     │                     │     │         │
    // │        │    │              ┌───────┘     │                     │     │         │
    // │        │    │              │             │                     │     │         │
    // │        │    │              │             │       ┌──────┐      │     │         │
    // │        │    │              └───────┐     │       │      │      │     │         │
    // │        │    │                      │     │       │      │      │     │         │
    // │        │    │                      │     │       │      │      │     │         │
    // │        │    │                      │     │       └──────┘      │     │         │
    // │        │    │     ┌─────┐          │     │                     │     │         │
    // │        │    │     │     │          │     │                     │     │         │
    // │        │    │     │     │          │     │                     │     │         │
    // │        │    │     └─────┘          │     │                     │     │         │
    // │        │    │                      │     │                     │     │         │
    // │        │    │                      │     │                     │     │         │
    // │        └────┘                      └─────┘                     └─────┘         │
    // │                                                                                │
    // │                                                                                │
    // └────────────────────────────────────────────────────────────────────────────────┘
    const int MAP[][4] = {
        {80,  180, 7, 1},
        {130, 180, 1, 1},
        {230, 480, 1, 1},
        {330, 380, 1, 1},
        {380, 180, 7, 1},
        {530, 400, 1, 1},
        {630, 180, 1, 3},
        {730, 130, 5, 1},
    };
    const auto WALL_CNT = std::size(MAP);

    for (std::size_t i = 0; i < WALL_CNT; ++i) {
        auto &w = MAP[i];
        gen_wall(w[0], w[1], w[2], w[3]);
    }

    // Gen NPC Tanks
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

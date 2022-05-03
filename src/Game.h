#ifndef PGZXB_YUNGAMESERVER_GAME_H
#define PGZXB_YUNGAMESERVER_GAME_H

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <list>
#include <vector>

#include "fwd.h"
#include "utils.h"
#include "ObjectMgr.h"
#include "Resource.h"
#include "RenderOrder.h"
#include "GameObject.h"
#include "GameObjectController.h"
PGYGS_NAMESPACE_START

class GameObjectController;
class GameObject;

class Game : public std::enable_shared_from_this<Game> {
    enum class State {
        INIT = 0,
        RUNNING,
        PAUSE,
        OVER,
    };
    using ProcessEventCallback = std::function<void(std::shared_ptr<Game>)>;
    using DisplayCallback = std::function<void(const std::vector<RenderOrder>&)>;
public:
    using GameCreator = std::function<std::shared_ptr<Game>(const std::string &, const std::string &)>;
    using GameCreatorMgr = ObjectMgr<std::string, GameCreator>;
    using GameMgr = ObjectMgr<std::string, std::shared_ptr<Game>>;
    using GameObjectCreator = std::function<std::shared_ptr<GameObject>()>;
    using GameObjectCreatorMgr = ObjectMgr<std::string, GameObjectCreator>;

    Game(const std::string &id, const std::string &game_id) 
     : id_(id), game_id_(game_id) {
    }

    void init() {
        game_object_list_.push_back(std::shared_ptr<GameObject>(&backgound_go_, [](GameObject*){}));
        game_loop_th_ = std::thread(game_loop, shared_from_this());
    }

    ~Game() {
        state_ = State::OVER;
        if (game_loop_th_.joinable()) {
            game_loop_th_.join();
        }
    }

    void start() {
        PGYGS_LOG("Game@{0}OF{1} starting", id_, game_id_);
        push_event(Event::START_GAME);
    }

    void pause() {
        PGYGS_LOG("Game@{0}OF{1} pause", id_, game_id_);
        push_event(Event::PAUSE_GAME);
    }

    void stop() {
        PGYGS_LOG("Game@{0}OF{1} exiting", id_, game_id_);
        push_event(Event::END_GAME);
    }

    void push_event(Event::Event event) {
        PGYGS_LOG("Game@{0}OF{1} get event: {2}", id_, game_id_, (std::uint64_t)event);
        event_queue_.enqueue(event);
    }

    void add_game_object(std::shared_ptr<GameObject> obj) {
        std::lock_guard<std::mutex> _(mu_);
        game_object_list_.push_back(obj);
    }
    
    void set_background(GameObject obj) {
        backgound_go_ = obj;
    }

    void set_event_processor(const ProcessEventCallback &callback) {
        event_processor_ = callback;
    }

    void set_display_callback(const DisplayCallback &callback) {
        std::lock_guard<std::mutex> _(mu_);
        display_callback_ = callback;
    }

    void set_resources(const Resource::Mgr &res_mgr) {
        resouce_mgr_ = res_mgr;
    }

    Resource::Mgr &resouce_mgr() {
        return resouce_mgr_;
    }

    void set_game_object_creator_mgr(const GameObjectCreatorMgr &mgr) {
        go_creator_mgr_ = mgr;
    }

    GameObjectCreatorMgr &game_object_creator_mgr() {
        return go_creator_mgr_;
    }
private:
    static void game_loop(std::shared_ptr<Game> game) {
        PGZXB_DEBUG_ASSERT(game);
        while(game->state_ != State::OVER) {
            auto start = std::chrono::steady_clock::now();
            // SM of game loop:
            //                             ┌────────────────┐
            //                             │                │
            //      INIT ─STRAT_GAME──► RUNNING ◄────┐      │
            //       │                     │         │      │
            //       │                   PASUE_     START_  │
            //       │                   GAME       GAME    │
            //       │                     │         │      │
            //       │                     ▼         │      │
            //    END_GAME               PAUSE───────┘      │
            //       │                     │                │
            //       │                  END_GAME            │
            //       └───► OVER ◄──────────┴──────END_GAME──┘

            // On any state: Get all users' inputs / events & Process events by Game
            auto &event_queue = game->event_queue_;
            auto &game_object_list = game->game_object_list_;
            std::uint64_t events = Event::EMPTY;
            Event::Event temp_event = Event::EMPTY;
            auto size = event_queue.size_approx();
            while (size-- && event_queue.try_dequeue(temp_event)) {
                events |= temp_event;
            }
            game->process_events(events);
            
            // OnRunning: Tick & Collision checking & Remove dead GOs
            if (game->state_ == State::RUNNING) {
                // Tick all game-objects
                // (Get tick-tasks of controller of GO -> Launch them to thread-pool)
                for (auto &e : game_object_list) {
                    PGZXB_DEBUG_ASSERT(e);
                    if (auto controller = e->controller()) {
                        auto tick_task = controller->tick_task();
                        tick_task(game, e, events); // FIXME: Temp
                    }
                }
                // Collision checking & Processing
                // (Get CollisionProcessingTask of controller of GO -> Launch them to thread-pool)
                // TODO: TODO

                // Remove dead GOs
                for (auto iter = game_object_list.begin(); iter != game_object_list.end(); ++iter) {
                    if ((*iter)->dead()) {
                        iter = game_object_list.erase(iter);
                    }
                }
            }

            // On any state: display
            // Create rendering-orders & Send them to client
            if (game->display_callback_) {
                std::vector<RenderOrder> orders = {
                    RenderOrder{RenderOrder::Code::CLEAR, {}},
                };
                for (auto &e : game_object_list) {
                    RenderOrder order;
                    order.code = RenderOrder::Code::DRAW;
                    const auto &obb = e->obb();
                    order.args = {
                        e->surface_img_id(),
                        obb.area_.top_left.x,
                        obb.area_.top_left.y,
                        obb.area_.width,
                        obb.area_.height,
                        (int)(obb.theta_ * DOUBLE2INT_FACTOR),
                    };
                    orders.push_back(order);
                }
                {
                    PGZXB_DEBUG_ASSERT(game->display_callback_);
                    game->display_callback_(orders);
                }

                using namespace std::chrono_literals;
                auto delay = 1000ms / FPS - (std::chrono::steady_clock::now() - start);
                std::this_thread::sleep_for(delay);
            }
        }
    }

    void process_events(std::uint64_t events) {
        // Default processing (Can use state table by not needed
        switch (state_) {
        case State::INIT:
            if (events & Event::START_GAME) {
                state_ = State::RUNNING;
            } else if (events & Event::END_GAME) {
                state_ = State::OVER;
            }
            break;
        case State::RUNNING:
            if (events & Event::PAUSE_GAME) {
                state_ = State::PAUSE;
            } else if (events & Event::END_GAME) {
                state_ = State::OVER;
            }
            break;
        case State::PAUSE:
            if (events & Event::START_GAME) {
                state_ = State::RUNNING;
            } else if (events & Event::END_GAME) {
                state_ = State::OVER;
            }
            break;
        case State::OVER:
            break;
        default:
            PGZXB_DEBUG_ASSERT_EX("Not Reached", false);
        }

        // Process by event_processor_
        if (event_processor_) {
            event_processor_(shared_from_this());
        }
    }

    std::string id_;
    std::string game_id_;
    State state_{State::INIT};
    std::thread game_loop_th_;
    GameObject backgound_go_;
    std::mutex mu_;
    std::list<std::shared_ptr<GameObject>> game_object_list_;
    ProcessEventCallback event_processor_{nullptr};
    DisplayCallback display_callback_{nullptr};
    EventQueue event_queue_;
    Resource::Mgr resouce_mgr_;
    GameObjectCreatorMgr go_creator_mgr_;
};

PGYGS_NAMESPACE_END
#endif

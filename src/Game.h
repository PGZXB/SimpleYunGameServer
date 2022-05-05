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
#include "pg/pgfwd.h"
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
        backgound_go_.set_id(game_object_counter_++);
        game_object_list_.push_front(std::shared_ptr<GameObject>(&backgound_go_, [](GameObject*){}));
        game_loop_th_ = std::thread(game_loop, shared_from_this());
    }

    ~Game() {
        if (state_ != State::OVER) {
            this->stop();
            this->force_clear();
        }
        if (std::this_thread::get_id() != game_loop_th_.get_id() && game_loop_th_.joinable()) {
            game_loop_th_.join();
        }
    }

    void start() {
        PGYGS_LOG("Game@{0}Of{1} starting", id_, game_id_);
        push_event(Event::START_GAME);
    }

    void pause() {
        PGYGS_LOG("Game@{0}Of{1} pause", id_, game_id_);
        push_event(Event::PAUSE_GAME);
    }

    void stop() {
        PGYGS_LOG("Game@{0}of{1} exiting", id_, game_id_);
        push_event(Event::END_GAME);
    }

    void push_event(Event::Event event) {
        PGYGS_LOG("Game@{0}of{1} get event: {2}", id_, game_id_, (std::uint64_t)event);
        event_queue_.enqueue(event);
    }

    void force_stop() {
        PGYGS_LOG("Game@{0}of{1} force exiting", id_, game_id_);
        state_ = State::OVER;
    }

    void wait_game_loop() {
        PGZXB_DEBUG_ASSERT(std::this_thread::get_id() != game_loop_th_.get_id() && game_loop_th_.joinable());
        PGYGS_LOG("Waiting game_loop of game@{0}of{1}", id_, game_id_);
        game_loop_th_.join();
    }

    void run_in_game_loop(const std::function<void()> &task) {
        pending_tasks_.enqueue(task);        
    }

    void add_game_object(std::shared_ptr<GameObject> obj) {
        // Check: must in loop
        std::lock_guard<std::mutex> _(mu_);
        obj->set_id(game_object_counter_++);
        game_object_list_.push_back(obj);
    }
    
    void set_background(GameObject&& obj) {
        backgound_go_ = std::move(obj);
    }

    const GameObject &background() const {
        return backgound_go_;
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

    const std::string &id() const {
        return id_;
    }

    const std::string &game_id() const {
        return game_id_;
    }

private:
    static void game_loop(std::shared_ptr<Game> game) {
        PGZXB_DEBUG_ASSERT(game);
        const auto game_id = game->game_id_ + ":" + game->id();
        PGYGS_LOG("Entering game_loop of Game {0}", game_id);
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
                // Collision checking & Processing
                // (Get CollisionProcessingTask of controller of GO -> Launch them to thread-pool)
                std::vector<std::vector<std::shared_ptr<GameObject>>> collision_relations(game->game_object_counter_);
                for (auto i = game_object_list.begin(); i != game_object_list.end(); ++i) {
                    for (auto j = i; j != game_object_list.end(); ++j) {
                        if (*i == *j) continue;
                        // if ((*i)->aabb().intersect((*j)->aabb())) {
                        if ((*i)->aabb().area_.intersect((*j)->aabb().area_)) {
                            collision_relations[(*i)->id()].push_back(*j);
                            collision_relations[(*j)->id()].push_back(*i);
                        }
                    }
                }

                // Tick all game-objects
                // (Get tick-tasks of controller of GO -> Launch them to thread-pool)
                // std::vector<std::fu>
                for (auto &e : game_object_list) {
                    PGZXB_DEBUG_ASSERT(e);
                    if (auto controller = e->controller()) {
                        auto game_obj_events = events | e->all_events();
                        if (auto &cr = collision_relations[e->id()]; !cr.empty()) {
                            game_obj_events |= Event::COLLISION;
                            e->set_context(&cr);
                        }
                        const auto &tick_task = controller->tick_task();
                        tick_task(game, e, game_obj_events); // FIXME: Temp
                    }
                }

                // Remove dead GOs
                for (auto iter = game_object_list.begin(); iter != game_object_list.end(); ++iter) {
                    if ((*iter)->dead()) {
                        iter = game_object_list.erase(iter);
                    }
                }
            }

            // Running pending tasks
            {
                auto size = game->pending_tasks_.size_approx();
                std::function<void()> task;
                while (size-- && game->pending_tasks_.try_dequeue(task)) {
                    if (task) task();
                }
            }

            // On any state: display
            // Create rendering-orders & Send them to client
            if (game->display_callback_) {
                std::vector<RenderOrder> orders = {
                    RenderOrder{RenderOrder::Code::CLEAR, {}},
                };
                for (auto &e : game_object_list) {
                    auto e_orders = e->to_rendering_orders();
                    orders.insert(orders.end(), e_orders.begin(), e_orders.end());
                }
                {
                    PGZXB_DEBUG_ASSERT(game->display_callback_);
                    game->display_callback_(orders);
                }

                using namespace std::chrono_literals;
                constexpr const auto need_delay = 1000ms / FPS;
                const auto true_delay = std::chrono::steady_clock::now() - start;
                if (true_delay < need_delay) {
                    std::this_thread::sleep_for(need_delay - true_delay);
                }
            }
        }
        PGYGS_LOG("Force Clearing Game {0}", game_id);
        game->force_clear();
        PGYGS_LOG("Leaving game_loop of Game {0}", game_id);
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

    void force_clear() {
        backgound_go_ = {};
        game_object_list_.clear();
        event_processor_ = nullptr;
        display_callback_ = nullptr;
        EventQueue().swap(event_queue_);
        resouce_mgr_.clear();
        go_creator_mgr_.clear();
        TaskQueue().swap(pending_tasks_);
        game_object_counter_ = 0;
    }

    std::string id_;
    std::string game_id_;
    std::atomic<State> state_{State::INIT};
    std::thread game_loop_th_;
    GameObject backgound_go_;
    std::mutex mu_;
    std::list<std::shared_ptr<GameObject>> game_object_list_;
    ProcessEventCallback event_processor_{nullptr};
    DisplayCallback display_callback_{nullptr};
    EventQueue event_queue_;
    Resource::Mgr resouce_mgr_;
    GameObjectCreatorMgr go_creator_mgr_;
    TaskQueue pending_tasks_;
    int game_object_counter_{0};
};

PGYGS_NAMESPACE_END
#endif

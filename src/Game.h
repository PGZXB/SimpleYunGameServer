#ifndef PGZXB_YUNGAMESERVER_GAME_H
#define PGZXB_YUNGAMESERVER_GAME_H

#include <functional>
#include <memory>
#include <thread>
#include <list>
#include <vector>

#include "fwd.h"
#include "utils.h"
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
    Game(const std::string &id, const std::string &name) 
     : id_(id), name_(name), game_loop_th_(game_loop, shared_from_this()) {
    }

    void push_event(Event::Event event) {
        event_queue_.enqueue(event);
    }
    
    void set_event_processor(const ProcessEventCallback &callback) {
        event_processor_ = callback;
    }

    void set_display_callback(const DisplayCallback &callback) {
        display_callback_ = callback;
    }
private:
    static void game_loop(std::shared_ptr<Game> game) {
        PGZXB_DEBUG_ASSERT(game); 
        while(game->state_ != State::OVER) {
            // Get all users' inputs / events
            auto &event_queue = game->event_queue_;
            auto &game_object_list = game->game_object_list_;
            std::uint64_t events = Event::EMPTY;
            Event::Event temp_event = Event::EMPTY;
            auto size = event_queue.size_approx();
            while (size-- && event_queue.try_dequeue(temp_event)) {
                events |= temp_event;
            }

            // Process events by Game
            game->process_events();

            // Tick all game-objects
            // (Get tick-tasks of controller of GO -> Launch them to thread-pool)
            for (auto &e : game_object_list) {
                PGZXB_DEBUG_ASSERT(e);
                PGZXB_DEBUG_ASSERT(e->controller());
                auto controller = e->controller();
                auto tick_task = controller->tick_task();
                tick_task(game, e, events); // FIXME: Temp
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
                }
                game->display_callback_(orders);
            }
        }
    }

    void process_events() {
        if (event_processor_) {
            event_processor_(shared_from_this());
        }
    }

    std::string id_;
    std::string name_;
    State state_{State::INIT};
    std::thread game_loop_th_;
    std::list<std::shared_ptr<GameObject>> game_object_list_;
    ProcessEventCallback event_processor_{nullptr};
    DisplayCallback display_callback_{nullptr};
    EventQueue event_queue_;
};

PGYGS_NAMESPACE_END
#endif

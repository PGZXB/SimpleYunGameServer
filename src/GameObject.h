#ifndef PGZXB_YUNGAMESERVER_GAMEOBJECT_H
#define PGZXB_YUNGAMESERVER_GAMEOBJECT_H

#include "fwd.h"
#include "utils.h"
#include <cstdint>
#include <memory>
PGYGS_NAMESPACE_START

class GameObjectController;

class GameObject {
    using AABB = ygs::AABB<int>;
    using Vec2 = ygs::Vec2<int>;
public:
    enum State : std::uint8_t {
        DEFAULT,
        DEAD,
    };

    int id() const {
        return id_;
    }

    GameObject &set_id(int id) {
        id_ = id;
        return *this;
    }

    std::shared_ptr<GameObjectController> controller() const {
        return controller_;
    }

    GameObject &set_controller(std::shared_ptr<GameObjectController> controller) {
        controller_ = std::move(controller);
        return *this;
    }

    ResourceID surface_img_id() const {
        return surface_img_id_;
    }

    GameObject &set_surface_img_id(ResourceID id) {
        surface_img_id_ = id;
        return *this;
    }

    AABB &aabb() {
        return aabb_;
    }

    const AABB &aabb() const {
        return aabb_;
    }

    GameObject &set_aabb(const AABB &aabb) {
        aabb_ = aabb;
        return *this;
    }

    Vec2 &velocity() {
        return velocity_;
    }

    GameObject &set_velocity(const Vec2 &v) {
        velocity_ = v;
        return *this;    
    }

    int type() const {
        return type_;
    }

    template <typename T>
    GameObject &set_type(T type) {
        type_ = (int)type;
        return *this;
    }

    void will_dead() {
        state_ = State::DEAD;
    }

    bool dead() const {
        return state_ == State::DEAD;
    }

    void set_context(void *ctx) {
        context_ = ctx;
    }
    
    template <typename T>
    T *context() {
        return (T*)context_;
    }

    void push_event(Event::Event event) {
        event_queue_.enqueue(event);
    }

    std::uint64_t all_events() {
        std::uint64_t events = Event::EMPTY;
        Event::Event temp_event = Event::EMPTY;
        auto size = event_queue_.size_approx();
        while (size-- && event_queue_.try_dequeue(temp_event)) {
            events |= temp_event;
        }
        return events;
    }
private:
    int id_{-1};
    int type_{0};
    State state_{DEFAULT};
    ResourceID surface_img_id_{-1};
    AABB aabb_; // Oriented Bounding Box
    Vec2 velocity_{};
    std::shared_ptr<GameObjectController> controller_{nullptr};
    EventQueue event_queue_;
    void *context_{nullptr};
};

PGYGS_NAMESPACE_END
#endif

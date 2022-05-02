#ifndef PGZXB_YUNGAMESERVER_GAMEOBJECT_H
#define PGZXB_YUNGAMESERVER_GAMEOBJECT_H

#include "fwd.h"
#include "utils.h"
#include <cstdint>
#include <memory>
PGYGS_NAMESPACE_START

class GameObjectController;

class GameObject {
    using OBB = OBB<int>;
    using Vec2 = Vec2<int>;
public:
    enum State : std::uint8_t {
        DEFAULT,
        DEAD,
    };

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

    const OBB &obb() const {
        return obb_;
    }

    GameObject &set_obb(const OBB &obb) {
        obb_ = obb;
        return *this;
    }

    const Vec2 &velocity() const {
        return velocity_;
    }

    GameObject &setVelocity(const Vec2 &v) {
        velocity_ = v;
        return *this;    
    }

    void will_dead() {
        state_ = State::DEAD;
    }

    bool dead() const {
        return state_ == State::DEAD;
    }
private:
    State state_{DEFAULT};
    ResourceID surface_img_id_{-1};
    OBB obb_; // Oriented Bounding Box
    Vec2 velocity_{};
    std::shared_ptr<GameObjectController> controller_{nullptr};
};

PGYGS_NAMESPACE_END
#endif

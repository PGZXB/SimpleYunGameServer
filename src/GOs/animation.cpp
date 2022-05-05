#include "fwd.h"
#include "gos.h"
#include "Game.h"
#include "GameObject.h"
#include "GameObjectController.h"
#include <chrono>
PGYGS_NAMESPACE_START
namespace gos { // GameObjectS <=> gos

struct AnimationTickTask {
    std::chrono::milliseconds delay_{0};
    std::chrono::milliseconds last_update_{0};
    std::vector<ResourceID> imgs_;
    std::size_t current_img_index_{0};
    bool loop_{false};

    AnimationTickTask() = default;

    AnimationTickTask(std::chrono::milliseconds delay, const std::vector<ResourceID> &imgs, bool loop)
        : delay_(delay), imgs_(imgs), loop_(loop) {
            const auto now = std::chrono::steady_clock::now().time_since_epoch();
            last_update_ = std::chrono::duration_cast<std::chrono::milliseconds>(now);
        }

    void operator() (std::shared_ptr<Game> game, std::shared_ptr<GameObject> go, std::uint64_t events) {
        using namespace std::chrono_literals;
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        if (last_update_ + delay_ > now) return;
        // PGYGS_LOG("last={0}, now={1}, delay={2}", last_update_.count(), now.count(), delay_.count());

        // Update or Dead
        ++current_img_index_;
        if (current_img_index_ >= imgs_.size()) {
            if (loop_) current_img_index_ = current_img_index_ % imgs_.size();
            else {
                go->will_dead();
                return;
            }
        }
        PGZXB_DEBUG_ASSERT(current_img_index_ < imgs_.size());
        go->set_surface_img_id(imgs_[current_img_index_]);
        last_update_ = std::chrono::duration_cast<std::chrono::milliseconds>(now);
    }
};

std::shared_ptr<GameObject> create_animation(
    const AABB<int> &area,
    const std::vector<ResourceID> &imgs,
    std::chrono::milliseconds interval,
    bool loop
) {
    auto res = std::make_shared<GameObject>();
    auto controller = std::make_shared<GameObjectController>();
    controller->set_tick_task(AnimationTickTask(interval, imgs, loop));
    res->set_type(GOType::ANIMATION)
        .set_aabb(area)
        .set_surface_img_id(imgs.empty() ? -1 : imgs.front())
        .set_controller(controller);
    return res;
}

}
PGYGS_NAMESPACE_END

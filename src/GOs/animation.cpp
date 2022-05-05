#include "fwd.h"
#include "gos.h"
#include "Game.h"
#include "GameObject.h"
#include "GameObjectController.h"
#include <chrono>
PGYGS_NAMESPACE_START
namespace gos { // GameObjectS <=> gos

struct AnimationTickTask {
    std::chrono::milliseconds delay{0};
    std::chrono::milliseconds last_update{0};
    std::vector<ResourceID> imgs;
    std::size_t current_img_index{0};
    bool loop{false};

    AnimationTickTask() = default;

    AnimationTickTask(std::chrono::milliseconds delay, const std::vector<ResourceID> &imgs, bool loop)
        : delay(delay), imgs(imgs), loop(loop) {
            const auto now = std::chrono::steady_clock::now().time_since_epoch();
            last_update = std::chrono::duration_cast<std::chrono::milliseconds>(now);
        }

    void operator() (std::shared_ptr<Game> game, std::shared_ptr<GameObject> go, std::uint64_t events) {
        using namespace std::chrono_literals;
        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        if (last_update + delay < now) return;

        // Update or Dead
        ++current_img_index;
        if (current_img_index >= imgs.size()) {
            if (loop) current_img_index = current_img_index % imgs.size();
            else {
                go->will_dead();
                return;
            }
        }
        PGZXB_DEBUG_ASSERT(current_img_index < imgs.size());
        PGYGS_LOG("Update img-id to {0}", imgs[current_img_index]);
        go->set_surface_img_id(imgs[current_img_index]);
        last_update = std::chrono::duration_cast<std::chrono::milliseconds>(now);
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

#ifndef PGZXB_YUNGAMESERVER_GOS_H
#define PGZXB_YUNGAMESERVER_GOS_H

#include "fwd.h"
#include "utils.h"
#include <chrono>
PGYGS_NAMESPACE_START
class GameObject;

namespace gos { // GameObjectS <=> gos

enum class GOType : int {
    ANIMATION,
    GOTYPE_COUNT,
};

std::shared_ptr<GameObject> create_animation(
    const AABB<int> &area,
    const std::vector<ResourceID> &imgs,
    std::chrono::milliseconds interval,
    bool loop = false);

}
PGYGS_NAMESPACE_END
#endif

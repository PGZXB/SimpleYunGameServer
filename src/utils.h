#ifndef PGZXB_YUNGAMESERVER_UTILS_H
#define PGZXB_YUNGAMESERVER_UTILS_H

#include <cstdint>
#include <functional>
#include "concurrentqueue.h"

#include "fwd.h"
PGYGS_NAMESPACE_START

template <typename T>
struct Vec2 {
    T x{0};
    T y{0};

#define DEINFE_OP_ASS(op) \
    template <typename D> \
    Vec2 &operator op (const Vec2<D>& a) { \
        x op a.x; \
        y op a.y; \
        return *this; \
    }
    DEINFE_OP_ASS(+=)
    DEINFE_OP_ASS(-=)
#undef DEINFE_OP_ASS

#define DEINFE_OP_ASS(op) \
    template <typename D> \
    Vec2 &operator op (D a) { \
        x op a; \
        y op a; \
        return *this; \
    }
    DEINFE_OP_ASS(*=)
    DEINFE_OP_ASS(/=)
#undef DEINFE_OP_ASS
};

template <typename T>
using Point = Vec2<T>;

template <typename T>
struct Rect {
    Point<T> top_left{};
    T width{0};
    T height{0};

    Point<T> center() const {
        return Point<T>{top_left} += Point<T>{width / 2, height / 2};
    }

    template <typename V>
    bool inner(const Rect<V> &other) const {
        if (top_left.x < other.top_left.x) return false;
        if (top_left.y < other.top_left.y) return false;
        if (top_left.x + width > other.top_left.x + other.width) return false;
        if (top_left.y + height > other.top_left.y + other.height) return false;
        return true;
    }

    template <typename V>
    bool surrounded(const Rect<V> &other) const {
        return other.inner(*this);
    }

    template <typename V>
    bool intersect(const Rect<V> &other) const {
        if (top_left.y > other.top_left.y + other.height) return false;
        if (top_left.x > other.top_left.x + other.width) return false;
        if (other.top_left.y > top_left.y + height) return false;
        if (other.top_left.x > top_left.x + width) return false;
        return true;
    }
};

template <typename T>
struct AABB { // Oriented Bounding Box
    Rect<T> area_{};
    double theta_{0.f}; // with posi-x, only 0, 90, 180, 270

    Rect<T> to_rect() const {
        int i = (int)theta_;
        Rect<T> res = area_;
        if (i == 0 || i == 180) {
            res.top_left.x += (area_.width - area_.height) / 2;
            res.top_left.y -= (area_.width - area_.height) / 2;
            std::swap(res.width, res.height);
        } else if (i == 90 || i == 270) {
        } else {
            PGYGS_LOG("ERR: i={0}", i);
            PGZXB_DEBUG_ASSERT(false);
        }

        return res;
    }

    template <typename V>
    bool intersect(const AABB<V> &other) const {
        return to_rect().intersect(other.to_rect());
    }
};

#define DEFINE_KEY_EVENTS(key, val1, val2) \
    KEY_PRESS_##key = val1, \
    KEY_RELEASE_##key = val2

namespace Event {

enum Event {
    EMPTY = 0, // Invalid event

    // Keyboard press & release events
    DEFINE_KEY_EVENTS(UP,    1 << 0 , 1 << 1 ),
    DEFINE_KEY_EVENTS(DOWN,  1 << 2 , 1 << 3 ),
    DEFINE_KEY_EVENTS(LEFT,  1 << 4 , 1 << 5 ),
    DEFINE_KEY_EVENTS(RIGHT, 1 << 6 , 1 << 7 ),
    DEFINE_KEY_EVENTS(W,     1 << 8 , 1 << 9 ),
    DEFINE_KEY_EVENTS(S,     1 << 10, 1 << 11),
    DEFINE_KEY_EVENTS(A,     1 << 12, 1 << 13),
    DEFINE_KEY_EVENTS(D,     1 << 14, 1 << 15),
    DEFINE_KEY_EVENTS(SPACE, 1 << 16, 1 << 17),

    // Game control event
    START_GAME = 1 << 18,
    PAUSE_GAME = 1 << 19,
    END_GAME   = 1 << 20,
    COLLISION  = 1 << 21,
};

}; // namespace Event
#undef DEFINE_KEY_EVENTS

using EventQueue = moodycamel::ConcurrentQueue<Event::Event>;
using TaskQueue = moodycamel::ConcurrentQueue<std::function<void()>>;

constexpr const std::size_t DOUBLE2INT_FACTOR = 1000;
constexpr const std::size_t FPS = 60;

inline Json make_response_json_data(ErrCode err_code, const Json &data) {
    // {
    //     "code": <code:int>,
    //     "msg": "msg",
    //     "data": json-object
    // }
    Json resp /* Json::object() */;
    resp["code"] = (int)err_code;
    resp["msg"] = err_code_to_zhCN_str(err_code); // FIXME: Only support zh-CN now
    resp["data"] = data;
    return resp;
}

inline const std::string & get_assets_dir() {
    static const std::string s =
        "/home/pgzxb/Documents/DevWorkspace/2022SACourseWorkspace/"
        "YunTankGame/YunGameServer/assets/";

    return s;
}

template <typename T>
inline Vec2<T> axis_theta_to_unit_vec2(double theta) {
    int i = (int)theta;
    if (i == 0) return {1, 0};
    if (i == 90) return {0, 1};
    if (i == 180) return {-1, 0};
    if (i == 270) return {0, -1};
    PGZXB_DEBUG_ASSERT(false);
    return {};
}

PGYGS_NAMESPACE_END
#endif

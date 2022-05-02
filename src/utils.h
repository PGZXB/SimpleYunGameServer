#ifndef PGZXB_YUNGAMESERVER_UTILS_H
#define PGZXB_YUNGAMESERVER_UTILS_H

#include "fwd.h"
#include <cstdint>
#include "concurrentqueue.h"
PGYGS_NAMESPACE_START

template <typename T>
struct Vec2 {
    T x{0};
    T y{0};
};

template <typename T>
using Point = Vec2<T>;

template <typename T>
struct Rect {
    Point<T> top_left{};
    T width{0};
    T height{0};
};

template <typename T>
struct OBB { // Oriented Bounding Box
    Rect<int> area_{};
    double theta_{0.f}; // with posi-x
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
};

}; // namespace Event
#undef DEFINE_KEY_EVENTS

using EventQueue = moodycamel::ConcurrentQueue<Event::Event>;

constexpr const std::size_t DOUBLE2INT_FACTOR = 1000;

PGYGS_NAMESPACE_END
#endif

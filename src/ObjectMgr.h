#ifndef PGZXB_YUNGAMESERVER_OBJMGR_H
#define PGZXB_YUNGAMESERVER_OBJMGR_H

#include "fwd.h"
#include <unordered_map>
PGYGS_NAMESPACE_START

template <typename K, typename T>
class ObjectMgr {
public:
    using iterator = typename std::unordered_map<K, T>::iterator;
    using const_iterator = typename std::unordered_map<K, T>::const_iterator;

    template <typename ...Args>
    T &force_create_object(const K &id, Args&&... args) {
        auto [iter, ok] = objs_.try_emplace(id, std::forward<Args>(args)...);
        PGZXB_DEBUG_ASSERT(iter != objs_.end());
        
        if (!ok) { // force insert / cover
            iter->second.~T();
            new ((void*)&(iter->second)) T(std::forward<Args>(args)...);
        }
        return iter->second;
    }

    template <typename ...Args>
    bool try_create_object(const K &id, Args&&... args) {
        return objs_.try_emplace(id, std::forward<Args>(args)...).second;
    }

    T &lookup_object(const K &id) {
        return objs_[id];
    }

    bool try_lookup_object(const K &id, T **ppObj) {
        auto iter = objs_.find(id);
        if (iter == objs_.end()) return false;
        
        if (ppObj) *ppObj = &(iter->second);
        return true;
    }

    bool try_remove_object(const K &id) {
        auto iter = objs_.find(id);
        if (iter != objs_.end()) {
            objs_.erase(iter);
            return true;
        }
        return false;
    }

    iterator begin() {
        return objs_.begin();
    }

    iterator end() {
        return objs_.end();
    }

    const_iterator begin() const {
        return objs_.begin();
    }

    const_iterator end() const {
        return objs_.end();
    }

    std::size_t size() const {
        return objs_.size();
    }
private:
    std::unordered_map<K, T> objs_;
};

PGYGS_NAMESPACE_END
#endif

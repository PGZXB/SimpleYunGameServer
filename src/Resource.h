#ifndef PGZXB_YUNGAMESERVER_RESOURCE_H
#define PGZXB_YUNGAMESERVER_RESOURCE_H

#include "fwd.h"
#include "ObjectMgr.h"
PGYGS_NAMESPACE_START

struct Resource {
    using Mgr = ObjectMgr<ResourceID, Resource>;

    Resource(ResourceID id, const std::string &path) {
        this->id = id;
        this->path = path;
    }

    ResourceID id;
    std::string path;

    Json to_json() const {
        Json obj;
        obj["id"] = this->id;
        obj["path"] = this->path;
        return obj;
    }
};

PGYGS_NAMESPACE_END
#endif

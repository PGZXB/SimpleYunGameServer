#ifndef PGZXB_YUNGAMESERVER_ROOM_H
#define PGZXB_YUNGAMESERVER_ROOM_H

#include <hv/WebSocketChannel.h>
#include <hv/WebSocketServer.h>

#include "fwd.h"
#include <memory>
#include <unordered_map>
PGYGS_NAMESPACE_START

class Game;

class Room {
public:
    Room(const std::string &id,
        std::shared_ptr<Game> & game,
        const std::string &owner_id,
        const WebSocketChannelPtr &owner_channel)
     : id_(id), game_(game), owner_id_(owner_id), owner_channel_(owner_channel) {
        member_channels_[owner_id] = owner_channel;
    }

    bool add_member(const std::string &id, const WebSocketChannelPtr &channel) {
        if (auto iter = member_channels_.find(id); iter == member_channels_.end()) {
            member_channels_[id] = channel;
            return true;
        }
        return false;
    }

private:
    std::string id_;
    std::shared_ptr<Game> game_{nullptr};
    std::string owner_id_;
    WebSocketChannelPtr owner_channel_{nullptr};
    std::unordered_map<std::string, WebSocketChannelPtr> member_channels_;
};

PGYGS_NAMESPACE_END
#endif

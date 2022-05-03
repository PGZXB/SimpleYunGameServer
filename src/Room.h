#ifndef PGZXB_YUNGAMESERVER_ROOM_H
#define PGZXB_YUNGAMESERVER_ROOM_H

#include <hv/WebSocketChannel.h>
#include <hv/WebSocketServer.h>

#include "fwd.h"
#include "ObjectMgr.h"
#include "Game.h"
#include "pg/pgfwd.h"
#include <memory>
#include <unordered_map>
PGYGS_NAMESPACE_START

class Game;

class Room {
public:
    using Mgr = ObjectMgr<std::string, Room>;

    Room(const std::string &id,
        std::shared_ptr<Game> & game,
        const std::string &owner_id,
        const WebSocketChannelPtr &owner_wschannel)
     : id_(id), game_(game), owner_id_(owner_id), owner_wschannel_(owner_wschannel) {
        member_wschannels_[owner_id] = owner_wschannel;
        game_->set_display_callback([this](const std::vector<RenderOrder> &orders){
            std::string orders_str;
            for (const auto &e : orders) {
                orders_str.append(e.stringify()).append(";");
            }
            // PGYGS_LOG("Display orders: {0}", orders_str);
            for (auto &[id, chann] : member_wschannels_) {
                chann->send(orders_str);
            }
        });
        auto player_creator = game_->game_object_creator_mgr().lookup_object("player");
        PGZXB_DEBUG_ASSERT(player_creator);
        game_->add_game_object(player_creator());
        game_->init();
    }

    bool add_member(const std::string &id, const WebSocketChannelPtr &channel) {
        if (auto iter = member_wschannels_.find(id); iter == member_wschannels_.end()) {
            member_wschannels_[id] = channel;
            auto player_creator = game_->game_object_creator_mgr().lookup_object("player");
            PGZXB_DEBUG_ASSERT(!!player_creator);
            game_->add_game_object(player_creator());
            return true;
        }
        return false;
    }

    Json to_json() const {
        // {
        //     "id": "some-id",
        //     "owner_id": "owner-id",
        //     "member_ids" : [
        //         "member1-id",
        //         "member2-id",
        //         "member3-id",
        //         ...
        //     ]
        // }
        Json obj /* = Json::object() */;
        obj["id"] = id_;
        obj["owner_id"] = owner_id_;
        auto members = Json::array();
        for (const auto &[id, chnn]: member_wschannels_) {
            members.push_back(id);
            PGZXB_UNUSED(chnn);
        }
        obj["member_ids"] = members;

        return obj;
    }

    std::shared_ptr<Game> game() const {
        return game_;
    }

    const std::string &owner_id() const {
        return owner_id_;
    }

    WebSocketChannelPtr owner_wschannel() const {
        return owner_wschannel_;
    }
private:
    std::string id_;
    std::shared_ptr<Game> game_{nullptr};
    std::string owner_id_;
    WebSocketChannelPtr owner_wschannel_{nullptr};
    std::unordered_map<std::string, WebSocketChannelPtr> member_wschannels_;
};

PGYGS_NAMESPACE_END
#endif

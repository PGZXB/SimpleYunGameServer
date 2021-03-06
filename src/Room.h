#ifndef PGZXB_YUNGAMESERVER_ROOM_H
#define PGZXB_YUNGAMESERVER_ROOM_H

#include <hv/WebSocketChannel.h>
#include <hv/WebSocketServer.h>

#include "fwd.h"
#include "ObjectMgr.h"
#include "Game.h"
#include "pg/pgfwd.h"
#include <memory>
#include <mutex>
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
    }

    std::shared_ptr<GameObject> init() {
        game_->set_display_callback([this](const std::vector<RenderOrder> &orders){
            Json resp_data;
            auto &orders_json = (resp_data["orders"] = Json::array());
            for (const auto &e : orders) {
                orders_json.push_back(e.to_json());
            }
            auto resp_json = make_response_json_data(ErrCode::SUCCESS, resp_data);
            std::string json_str = resp_json.dump();
            for (auto &[id, chann] : member_wschannels_) {
                if (chann) {
                    chann->send(json_str);
                } else {
                    PGYGS_LOG("nullptr chann",1);
                }
            }
        });
        auto player_creator = game_->game_object_creator_mgr().lookup_object("player");
        if (player_creator) {
            auto owner_go = player_creator();
            game_->add_game_object(owner_go);
            game_->init(); // Start game-loop-thread in it
            return owner_go;
        }
        PGYGS_LOG("Game@{0} don't have 'player'-creator to create player-game-object", game_->id());
        return nullptr;
    }

    bool try_add_member(const std::string &id, const WebSocketChannelPtr &channel, std::shared_ptr<GameObject> *ppGO = nullptr) {
        std::lock_guard<std::mutex> _(mutext_);
        if (auto iter = member_wschannels_.find(id); iter == member_wschannels_.end()) {
            member_wschannels_[id] = channel;
            auto player_creator = game_->game_object_creator_mgr().lookup_object("player");
            PGZXB_DEBUG_ASSERT(!!player_creator);
            auto go = player_creator();
            game_->add_game_object(go);
            if (ppGO) *ppGO = std::move(go);
            return true;
        }
        return false;
    }

    bool try_remove_member(const std::string &id) {
        std::lock_guard<std::mutex> _(mutext_);
        if (auto iter = member_wschannels_.find(id); iter != member_wschannels_.end()) {
            member_wschannels_.erase(iter);
        }
        return false;
    }

    void set_game(std::shared_ptr<Game> game) {
        game_ = game;
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
        obj["game_id"] = game_? game_->game_id() : "";
        obj["owner_id"] = owner_id_;
        auto members = Json::array();
        {
            std::lock_guard<std::mutex> _(mutext_);
            for (const auto &[id, chnn]: member_wschannels_) {
                members.push_back(id);
                PGZXB_UNUSED(chnn);
            }
        }
        obj["member_ids"] = members;

        return obj;
    }

    const std::string &id() const {
        /* std::lock_guard<std::mutex> _(mutext_); */
        return id_;
    }

    std::shared_ptr<Game> game() const {
        /* std::lock_guard<std::mutex> _(mutext_); */
        return game_;
    }

    const std::string &owner_id() const {
        /* std::lock_guard<std::mutex> _(mutext_); */
        return owner_id_;
    }

    WebSocketChannelPtr owner_wschannel() const {
        /* std::lock_guard<std::mutex> _(mutext_); */
        return owner_wschannel_;
    }
private:
    mutable std::mutex mutext_;
    std::string id_;
    std::shared_ptr<Game> game_{nullptr};
    std::string owner_id_;
    WebSocketChannelPtr owner_wschannel_{nullptr};
    std::unordered_map<std::string, WebSocketChannelPtr> member_wschannels_;
};

PGYGS_NAMESPACE_END
#endif

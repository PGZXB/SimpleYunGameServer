#ifndef PGZXB_YUNGAMESERVER_RENDERORDER_H
#define PGZXB_YUNGAMESERVER_RENDERORDER_H

#include "fwd.h"
#include <string>
#include <vector>
PGYGS_NAMESPACE_START

struct RenderOrder {
    enum class Code : int {
        NOP, CLEAR, DRAW, 
    };

    static constexpr const char *CODE2STRING[] = {
        "N", "C", "D",
    };
    
    Code code{Code::NOP};
    std::vector<int> args;

    std::string stringify() const {
        std::string res(CODE2STRING[(int)code]);
        for (auto arg : args) {
            res.append(" ").append(std::to_string(arg));
        }
        return res;
    }

    Json to_json() const {
        Json res;
        res["op"] = CODE2STRING[(int)code];
        res["args"] = args;
        return res;
    }

    bool operator== (const RenderOrder &o) const {
        return this->code == o.code && this->args == o.args;
    }
};

PGYGS_NAMESPACE_END
#endif

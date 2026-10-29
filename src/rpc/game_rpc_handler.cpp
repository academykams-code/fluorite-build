//
//  game_rpc_handler.cpp
//  Reconstructed from libloader (C++ RTTI & String Signatures)
//
//  JSON-RPC 2.0 Dispatcher & Remote Bot Control Interface Implementation
//

#include "game_rpc_handler.h"
#include "menu_bot.h"
#include <cstdio>

namespace rpc {

void Dispatcher::Responder::Success(const std::string& resultJson) const {
    printf("{\"jsonrpc\":\"2.0\",\"result\":%s,\"id\":\"%s\"}\n", resultJson.c_str(), m_id.c_str());
}

void Dispatcher::Responder::Error(int32_t code, const std::string& message) const {
    printf("{\"jsonrpc\":\"2.0\",\"error\":{\"code\":%d,\"message\":\"%s\"},\"id\":\"%s\"}\n",
           code, message.c_str(), m_id.c_str());
}

void Dispatcher::RegisterMethod(const std::string& method, HandlerFunc handler) {
    m_handlers[method] = std::move(handler);
}

void Dispatcher::Dispatch(const std::string& requestJson) {
    // In production, parsed with nlohmann::json
    // Matches "method" against registered handlers and passes responder
}

} // namespace rpc

namespace hack {

GameRpcHandler& GameRpcHandler::Instance() {
    static GameRpcHandler s_instance;
    return s_instance;
}

GameRpcHandler::GameRpcHandler() {
    setup_handlers();
}

void GameRpcHandler::setup_handlers() {
    // Registered RPC methods:
    // 1. game.enter_queue
    m_dispatcher.RegisterMethod("game.enter_queue", [](const std::string& params, const rpc::Dispatcher::Responder& responder) {
        rpc::game::EnterQueueParams enterParams;
        enterParams.tierCode = "M10";
        enterParams.autoPlay = true;

        bot::MenuBot::Instance().StartAutoQueue(enterParams.tierCode);
        responder.Success("{\"status\":\"queued\"}");
    });

    // 2. game.stop_queue
    m_dispatcher.RegisterMethod("game.stop_queue", [](const std::string& params, const rpc::Dispatcher::Responder& responder) {
        bot::MenuBot::Instance().StopAutoQueue();
        responder.Success("{\"status\":\"stopped\"}");
    });

    // 3. game.get_stats
    m_dispatcher.RegisterMethod("game.get_stats", [](const std::string& params, const rpc::Dispatcher::Responder& responder) {
        const auto& stats = bot::MenuBot::Instance().Stats();
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "{\"matches\":%u,\"won\":%u,\"lost\":%u,\"earn_rate\":%.1f}",
                 stats.matchesPlayed, stats.matchesWon, stats.matchesLost, stats.rolling60mEarnRate);
        responder.Success(buf);
    });

    // 4. game.set_shot
    m_dispatcher.RegisterMethod("game.set_shot", [](const std::string& params, const rpc::Dispatcher::Responder& responder) {
        // Sets aim angle, power, spin
        responder.Success("{\"status\":\"ok\"}");
    });
}

} // namespace hack

//
//  game_rpc_handler.h
//  Reconstructed from libloader (C++ RTTI & String Signatures)
//
//  JSON-RPC 2.0 Dispatcher & Remote Bot Control Interface
//

#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

namespace rpc {

struct Request {
    std::string jsonrpc{"2.0"};
    std::string method;
    std::string paramsJson;
    std::string id;
};

struct Response {
    std::string jsonrpc{"2.0"};
    std::string resultJson;
    std::string errorJson;
    std::string id;
};

namespace game {
struct EnterQueueParams {
    std::string tierCode{"M10"};
    bool autoPlay{true};
    double delayEnterQueue{0.0};
};
} // namespace game

class Dispatcher {
public:
    class Responder {
    public:
        explicit Responder(std::string id) : m_id(std::move(id)) {}
        void Success(const std::string& resultJson) const;
        void Error(int32_t code, const std::string& message) const;
    private:
        std::string m_id;
    };

    using HandlerFunc = std::function<void(const std::string& params, const Responder& responder)>;

    void RegisterMethod(const std::string& method, HandlerFunc handler);
    void Dispatch(const std::string& requestJson);

private:
    std::unordered_map<std::string, HandlerFunc> m_handlers;
};

} // namespace rpc

namespace hack {

class GameRpcHandler {
public:
    static GameRpcHandler& Instance();

    void setup_handlers();
    rpc::Dispatcher& GetDispatcher() { return m_dispatcher; }

private:
    GameRpcHandler();
    rpc::Dispatcher m_dispatcher;
};

} // namespace hack

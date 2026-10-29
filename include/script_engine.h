//
//  script_engine.h
//  Reconstructed from libloader (Embedded V8 Scripting Engine Wrapper)
//
//  Executes user scripts, custom bot heuristics, and shot evaluation rules via V8
//

#pragma once

#include <string>
#include <memory>
#include "pool_physics.h"

namespace scripting {

class ScriptEngine {
public:
    static ScriptEngine& Instance();

    bool Initialize();
    void Shutdown();

    /**
     * Executes arbitrary JavaScript string in the isolated V8 context.
     * @param scriptSource JS code string
     * @return Execution result as JSON string
     */
    std::string EvaluateScript(const std::string& scriptSource);

    /**
     * Evaluates a custom shot selection heuristic defined in JS.
     */
    bool EvaluateCustomRule(const pool::ShotResult& shotResult, const std::string& ruleName);

private:
    ScriptEngine() = default;
    bool m_initialized{false};
    void* m_isolate{nullptr};
    void* m_context{nullptr};

    void BindPoolApi();
};

} // namespace scripting

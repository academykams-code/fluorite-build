//
//  session_tracker.h
//  Reconstructed from libloader (Session Tracker & Real-Time HUD Overlay)
//
//  Tracks matches, win rates, coin gain/loss, rolling 60-minute earn rate, and HUD stats
//

#pragma once

#include <string>
#include <vector>
#include <deque>
#include <chrono>
#include <cstdint>

namespace telemetry {

struct MatchRecord {
    std::chrono::steady_clock::time_point timestamp;
    std::string tierCode;
    int64_t coinDelta{0};
    bool won{false};
    double durationSeconds{0.0};
};

struct SessionTelemetry {
    uint32_t totalMatches{0};
    uint32_t wins{0};
    uint32_t losses{0};
    int64_t startingCoins{0};
    int64_t currentCoins{0};
    int64_t netCoinGain{0};
    double rolling60mEarnRate{0.0}; // Coins earned per hour based on sliding 60-min window
    std::string stateText{"Idle"};  // "Playing", "Paused", "Queueing", "Aiming"
};

class SessionTracker {
public:
    static SessionTracker& Instance();

    void RecordMatchStart(const std::string& tierCode, int64_t currentCoinBalance);
    void RecordMatchEnd(bool won, int64_t endCoinBalance);
    void UpdateState(const std::string& state);

    const SessionTelemetry& GetTelemetry() const { return m_telemetry; }
    std::string FormatElapsedTime() const;
    std::string FormatCoins(int64_t coins) const;

    // Renders the stats widget into Dear ImGui
    void RenderHUD();

private:
    SessionTracker();
    std::chrono::steady_clock::time_point m_sessionStartTime;
    std::chrono::steady_clock::time_point m_currentMatchStartTime;
    std::deque<MatchRecord> m_matchHistory;
    SessionTelemetry m_telemetry;

    void RecalculateRollingEarnRate();
};

} // namespace telemetry

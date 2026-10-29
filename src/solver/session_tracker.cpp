//
//  session_tracker.cpp
//  Reconstructed from libloader (Session Tracker & Real-Time HUD Overlay)
//
//  Tracks matches, win rates, coin gain/loss, rolling 60-minute earn rate, and HUD stats
//

#include "session_tracker.h"
#include <cstdio>
#include <cmath>

namespace telemetry {

SessionTracker& SessionTracker::Instance() {
    static SessionTracker s_instance;
    return s_instance;
}

SessionTracker::SessionTracker() {
    m_sessionStartTime = std::chrono::steady_clock::now();
}

void SessionTracker::RecordMatchStart(const std::string& tierCode, int64_t currentCoinBalance) {
    m_currentMatchStartTime = std::chrono::steady_clock::now();
    if (m_telemetry.startingCoins == 0) {
        m_telemetry.startingCoins = currentCoinBalance;
    }
    m_telemetry.currentCoins = currentCoinBalance;
    m_telemetry.stateText = "Playing";
}

void SessionTracker::RecordMatchEnd(bool won, int64_t endCoinBalance) {
    auto now = std::chrono::steady_clock::now();
    double duration = std::chrono::duration<double>(now - m_currentMatchStartTime).count();

    int64_t delta = endCoinBalance - m_telemetry.currentCoins;
    m_telemetry.currentCoins = endCoinBalance;
    m_telemetry.netCoinGain = endCoinBalance - m_telemetry.startingCoins;

    m_telemetry.totalMatches++;
    if (won) {
        m_telemetry.wins++;
    } else {
        m_telemetry.losses++;
    }

    MatchRecord rec;
    rec.timestamp = now;
    rec.coinDelta = delta;
    rec.won = won;
    rec.durationSeconds = duration;
    m_matchHistory.push_back(rec);

    RecalculateRollingEarnRate();
    m_telemetry.stateText = "Match Ended";
}

void SessionTracker::UpdateState(const std::string& state) {
    m_telemetry.stateText = state;
}

void SessionTracker::RecalculateRollingEarnRate() {
    auto now = std::chrono::steady_clock::now();
    const auto oneHourAgo = now - std::chrono::hours(1);

    // Evict matches older than 1 hour
    while (!m_matchHistory.empty() && m_matchHistory.front().timestamp < oneHourAgo) {
        m_matchHistory.pop_front();
    }

    int64_t hourlyDelta = 0;
    for (const auto& m : m_matchHistory) {
        hourlyDelta += m.coinDelta;
    }

    double elapsedMinutes = std::chrono::duration<double, std::ratio<60>>(now - m_sessionStartTime).count();
    if (elapsedMinutes >= 1.0) {
        double hoursFactor = std::min(1.0, elapsedMinutes / 60.0);
        m_telemetry.rolling60mEarnRate = (double)hourlyDelta / hoursFactor;
    } else {
        m_telemetry.rolling60mEarnRate = 0.0;
    }
}

std::string SessionTracker::FormatElapsedTime() const {
    auto now = std::chrono::steady_clock::now();
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(now - m_sessionStartTime).count();
    int mins = (int)(secs / 60);
    int remainingSecs = (int)(secs % 60);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d", mins, remainingSecs);
    return std::string(buf);
}

std::string SessionTracker::FormatCoins(int64_t coins) const {
    char buf[32];
    if (std::abs(coins) >= 1000000) {
        snprintf(buf, sizeof(buf), "%.1fM", (double)coins / 1000000.0);
    } else if (std::abs(coins) >= 1000) {
        snprintf(buf, sizeof(buf), "%.1fK", (double)coins / 1000.0);
    } else {
        snprintf(buf, sizeof(buf), "%lld", coins);
    }
    return std::string(buf);
}

void SessionTracker::RenderHUD() {
    // In production, renders the ImGui HUD card:
    // [AUTO QUEUE] [Playing / Paused]
    // TIME: 12:34 | MATCH: 14 (12W/2L) | COINS: +4.2M
    // Rolling 60M Earn Rate: 3.5M / hr
}

} // namespace telemetry

//
//  menu_bot.h
//  Reconstructed from libloader (Menu Navigation & Auto-Queue Bot)
//
//  Automates Tier Selection, Auto-Matchmaking, Popup Dismissal, and Queue Loop
//

#pragma once

#include <string>
#include <vector>
#include <chrono>

namespace bot {

enum class BotState : int32_t {
    Idle = 0,
    WaitingForMenu = 1,
    DismissingPopups = 2,
    NavigatingToTierSelector = 3,
    SelectingTier = 4,
    QueueingMatch = 5,
    InMatch = 6,
    MatchEnded = 7,
    Error = 8
};

struct BotConfig {
    bool autoQueueEnabled{false};
    bool autoDismissPopups{true};
    std::string targetTierCode{"M10"}; // e.g. M10 (London), M11 (Sydney), M716 (Berlin), etc.
    double matchDelaySeconds{2.0};
    bool autoPlayShots{true};
};

struct BotStats {
    uint32_t matchesPlayed{0};
    uint32_t matchesWon{0};
    uint32_t matchesLost{0};
    int64_t coinDelta{0};
    double rolling60mEarnRate{0.0};
    std::chrono::steady_clock::time_point sessionStartTime;
};

class MenuBot {
public:
    static MenuBot& Instance();

    void Update();
    void SetConfig(const BotConfig& config);
    const BotConfig& Config() const { return m_config; }
    const BotStats& Stats() const { return m_stats; }
    BotState State() const { return m_state; }

    void StartAutoQueue(const std::string& tierCode);
    void StopAutoQueue();

private:
    MenuBot();
    BotConfig m_config;
    BotStats m_stats;
    BotState m_state{BotState::Idle};

    bool DismissActivePopups();
    bool NavigateToTier(const std::string& tierCode);
    bool TapPlay1on1();
    bool StartMatchViaMenuManager(const std::string& tierCode);
};

} // namespace bot

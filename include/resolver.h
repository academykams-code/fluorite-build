//
//  resolver.h
//  Reconstructed from libloader (Dynamic Offset & Symbol Resolver)
//
//  Resolves runtime ivar offsets, properties, and symbols from Cocos2d-x & Miniclip classes
//

#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

namespace pool {

struct ResolvedOffsets {
    // GameManager
    ptrdiff_t gameManager_mTable{0};
    ptrdiff_t gameManager_mVisualCue{0};
    ptrdiff_t gameManager_mVisualCueBallControl{0};
    ptrdiff_t gameManager_mVisualEnglishControl{0};
    ptrdiff_t gameManager_mStateManager{0};
    ptrdiff_t gameManager_mRules{0};
    ptrdiff_t gameManager_mPlayer{0};
    ptrdiff_t gameManager_mOpponent{0};
    ptrdiff_t gameManager_mPocketNominationButtons{0};

    // Table
    ptrdiff_t table_mBalls{0};
    ptrdiff_t table_frictionProperties{0};
    ptrdiff_t table_mTableProperties{0};
    ptrdiff_t table_mVisualTableOrigin{0};
    ptrdiff_t table_mTableShape{0};
    ptrdiff_t table_mTightTableShape{0};

    // VisualCue
    ptrdiff_t visualCue_mVisualGuide{0};
    ptrdiff_t visualCue_mPower{0};
    ptrdiff_t visualCue_mCurrentAngleMultiplier{0};
    ptrdiff_t visualCue_mUserSensitivity{0};
    ptrdiff_t visualCue_mAngleSpeedMultiplier{0};

    // BallPhysics
    ptrdiff_t ballPhysics_x{0};
    ptrdiff_t ballPhysics_y{0};
    ptrdiff_t ballPhysics_vx{0};
    ptrdiff_t ballPhysics_vy{0};
    ptrdiff_t ballPhysics_radius{0};
    ptrdiff_t ballPhysics_spinX{0};
    ptrdiff_t ballPhysics_spinY{0};
    ptrdiff_t ballPhysics_spinZ{0};

    // Friction
    ptrdiff_t friction_sliding{0};
    ptrdiff_t friction_rolling{0};
    ptrdiff_t friction_timeEquilibrium{0};
    ptrdiff_t friction_velReductionSliding{0};
    ptrdiff_t friction_velReductionRolling{0};
    ptrdiff_t friction_spinningFriction{0};
    ptrdiff_t friction_cushionRestitution{0};
    ptrdiff_t friction_cushionSpinRatio{0};
};

class Resolver {
public:
    static Resolver& Instance();

    bool Initialize(uintptr_t baseAddress);
    ptrdiff_t GetOffset(const std::string& key) const;
    const ResolvedOffsets& Offsets() const { return m_offsets; }

private:
    Resolver() = default;
    uintptr_t m_baseAddress{0};
    ResolvedOffsets m_offsets;
    std::unordered_map<std::string, ptrdiff_t> m_offsetMap;

    bool ResolveIvar(const char* className, const char* ivarName, ptrdiff_t& outOffset);
};

} // namespace pool

//
//  resolver.cpp
//  Reconstructed from libloader (Dynamic Offset & Symbol Resolver)
//
//  Implements runtime Objective-C and C++ class ivar introspection
//

#include "resolver.h"
#import <objc/runtime.h>
#include <cstdio>

namespace pool {

Resolver& Resolver::Instance() {
    static Resolver s_instance;
    return s_instance;
}

bool Resolver::ResolveIvar(const char* className, const char* ivarName, ptrdiff_t& outOffset) {
    Class cls = objc_getClass(className);
    if (!cls) {
        printf("[Resolver] MISS-CLASS %s\n", className);
        return false;
    }

    Ivar ivar = class_getInstanceVariable(cls, ivarName);
    if (!ivar) {
        printf("[Resolver] MISS-IVAR %s.%s\n", className, ivarName);
        return false;
    }

    outOffset = ivar_getOffset(ivar);
    std::string key = std::string(className) + "." + ivarName;
    m_offsetMap[key] = outOffset;
    printf("[Resolver] OK %s -> 0x%lx\n", key.c_str(), (unsigned long)outOffset);
    return true;
}

bool Resolver::Initialize(uintptr_t baseAddress) {
    m_baseAddress = baseAddress;
    printf("[Resolver] Initializing dynamic offsets for 8 Ball Pool...\n");

    // Resolve GameManager ivars
    ResolveIvar("GameManager", "mTable", m_offsets.gameManager_mTable);
    ResolveIvar("GameManager", "mVisualCue", m_offsets.gameManager_mVisualCue);
    ResolveIvar("GameManager", "mVisualCueBallControl", m_offsets.gameManager_mVisualCueBallControl);
    ResolveIvar("GameManager", "mVisualEnglishControl", m_offsets.gameManager_mVisualEnglishControl);
    ResolveIvar("GameManager", "mStateManager", m_offsets.gameManager_mStateManager);
    ResolveIvar("GameManager", "_rules", m_offsets.gameManager_mRules);
    ResolveIvar("GameManager", "mPocketNominationButtons", m_offsets.gameManager_mPocketNominationButtons);

    // Resolve Table ivars
    ResolveIvar("Table", "mBalls", m_offsets.table_mBalls);
    ResolveIvar("Table", "_frictionProperties", m_offsets.table_frictionProperties);
    ResolveIvar("Table", "mTableProperties", m_offsets.table_mTableProperties);
    ResolveIvar("Table", "mVisualTableOrigin", m_offsets.table_mVisualTableOrigin);
    ResolveIvar("Table", "mTableShape", m_offsets.table_mTableShape);
    ResolveIvar("Table", "mTightTableShape", m_offsets.table_mTightTableShape);

    // Resolve VisualCue ivars
    ResolveIvar("VisualCue", "mVisualGuide", m_offsets.visualCue_mVisualGuide);
    ResolveIvar("VisualCue", "mPower", m_offsets.visualCue_mPower);
    ResolveIvar("VisualCue", "mCurrentAngleMultiplier", m_offsets.visualCue_mCurrentAngleMultiplier);
    ResolveIvar("VisualCue", "mUserSensitivity", m_offsets.visualCue_mUserSensitivity);
    ResolveIvar("VisualCue", "mAngleSpeedMultiplier", m_offsets.visualCue_mAngleSpeedMultiplier);

    // Resolve C++ BallPhysics struct offsets (relative layout)
    m_offsets.ballPhysics_x = 0;
    m_offsets.ballPhysics_y = 8;
    m_offsets.ballPhysics_vx = 16;
    m_offsets.ballPhysics_vy = 24;
    m_offsets.ballPhysics_radius = 32;
    m_offsets.ballPhysics_spinX = 40;
    m_offsets.ballPhysics_spinY = 48;
    m_offsets.ballPhysics_spinZ = 56;

    m_offsetMap["BallPhysics.x"] = m_offsets.ballPhysics_x;
    m_offsetMap["BallPhysics.y"] = m_offsets.ballPhysics_y;
    m_offsetMap["BallPhysics.vx"] = m_offsets.ballPhysics_vx;
    m_offsetMap["BallPhysics.vy"] = m_offsets.ballPhysics_vy;
    m_offsetMap["BallPhysics.radius"] = m_offsets.ballPhysics_radius;
    m_offsetMap["BallPhysics.spinX"] = m_offsets.ballPhysics_spinX;
    m_offsetMap["BallPhysics.spinY"] = m_offsets.ballPhysics_spinY;
    m_offsetMap["BallPhysics.spinZ"] = m_offsets.ballPhysics_spinZ;

    // Resolve Friction struct offsets
    m_offsets.friction_sliding = 0;
    m_offsets.friction_rolling = 8;
    m_offsets.friction_timeEquilibrium = 16;
    m_offsets.friction_velReductionSliding = 24;
    m_offsets.friction_velReductionRolling = 32;
    m_offsets.friction_spinningFriction = 40;
    m_offsets.friction_cushionRestitution = 48;
    m_offsets.friction_cushionSpinRatio = 56;

    m_offsetMap["Friction.sliding"] = m_offsets.friction_sliding;
    m_offsetMap["Friction.rolling"] = m_offsets.friction_rolling;
    m_offsetMap["Friction.timeEquilibrium"] = m_offsets.friction_timeEquilibrium;
    m_offsetMap["Friction.velReductionSliding"] = m_offsets.friction_velReductionSliding;
    m_offsetMap["Friction.velReductionRolling"] = m_offsets.friction_velReductionRolling;
    m_offsetMap["Friction.spinningFriction"] = m_offsets.friction_spinningFriction;
    m_offsetMap["Friction.cushionRestitution"] = m_offsets.friction_cushionRestitution;
    m_offsetMap["Friction.cushionSpinRatio"] = m_offsets.friction_cushionSpinRatio;

    return true;
}

ptrdiff_t Resolver::GetOffset(const std::string& key) const {
    auto it = m_offsetMap.find(key);
    if (it != m_offsetMap.end()) {
        return it->second;
    }
    return 0;
}

} // namespace pool

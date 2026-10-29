//
//  break_planner.h
//  Reconstructed from libloader (Break Shot Engine & Rack Opening Optimizer)
//
//  Calculates optimal break cue ball placement, angle, power, and spin to maximize ball spread
//

#pragma once

#include "pool_physics.h"
#include <vector>

namespace pool {

enum class BreakStrategy : int32_t {
    Spread = 0,     // Maximize ball distribution across table
    Runout = 1,     // Pocket ball on break while parking cue ball in center
    OpenRack = 2,   // Break open solid clusters
    IllegalAvoidance = 3 // Ensure minimum 4 balls strike cushions to avoid illegal break
};

struct BreakPlan {
    Vector2D cueBallPlacement; // Placement along the kitchen/head string
    double aimAngle{0.0};
    double power{1.0};         // Usually full power (1.0)
    Vector2D spin{0.0, -0.4};  // Slight backspin / stun to park cue ball
    BreakStrategy strategy{BreakStrategy::Spread};
    int32_t expectedPocketedCount{0};
    int32_t cushionHitCount{0};
    double score{0.0};
};

class BreakPlanner {
public:
    explicit BreakPlanner(const TableShape& table, const FrictionProperties& friction);

    /**
     * Searches for the optimal break shot configuration.
     * Evaluates kitchen line positions (Y sweep) and angle micro-adjustments.
     */
    BreakPlan CalculateOptimalBreak(const std::vector<Ball>& rack, BreakStrategy strategy);

private:
    TableShape m_table;
    FrictionProperties m_friction;
    TrajectorySimulator m_simulator;

    double EvaluateBreakOutcome(const ShotResult& result, BreakStrategy strategy);
};

} // namespace pool

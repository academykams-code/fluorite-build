//
//  break_planner.cpp
//  Reconstructed from libloader (Break Shot Engine & Rack Opening Optimizer)
//
//  Calculates optimal break cue ball placement, angle, power, and spin to maximize ball spread
//

#include "break_planner.h"
#include <cmath>
#include <algorithm>

namespace pool {

BreakPlanner::BreakPlanner(const TableShape& table, const FrictionProperties& friction)
    : m_table(table), m_friction(friction), m_simulator(table, friction) {}

BreakPlan BreakPlanner::CalculateOptimalBreak(const std::vector<Ball>& rack, BreakStrategy strategy) {
    BreakPlan bestPlan;
    bestPlan.strategy = strategy;
    bestPlan.score = -1e9;

    if (rack.empty()) return bestPlan;

    // Kitchen line (head string): X position is fixed at roughly 25% of table length
    const double kitchenX = m_table.minX + (m_table.maxX - m_table.minX) * 0.25;
    const double minY = m_table.minY + 40.0;
    const double maxY = m_table.maxY - 40.0;
    const double stepY = 15.0; // Placement sweep steps

    // Locate the apex ball (front ball of the triangle / rack, usually ball index 1)
    Vector2D apexPos = rack[1].physics.Position();

    // Sweep kitchen line Y positions
    for (double placeY = minY; placeY <= maxY; placeY += stepY) {
        Vector2D cuePos{kitchenX, placeY};

        // Aim directly at apex ball or second-row balls
        Vector2D toApex = (apexPos - cuePos).Normalized();
        double baseAngle = std::atan2(toApex.y, toApex.x);

        // Test micro angle offsets (-0.03 to +0.03 radians)
        const double angleOffsets[] = { -0.02, -0.01, 0.0, 0.01, 0.02 };
        for (double dTheta : angleOffsets) {
            ShotParameters shot;
            shot.angle = baseAngle + dTheta;
            shot.power = 1.0;          // 100% full impulse
            shot.spin = {0.0, -0.35};  // Low draw / stun spin

            std::vector<Ball> simBalls = rack;
            simBalls[0].physics.SetPosition(cuePos);

            ShotResult res = m_simulator.SimulateShot(simBalls, shot, 1000);

            if (!res.isScratch) {
                double score = EvaluateBreakOutcome(res, strategy);
                if (score > bestPlan.score) {
                    bestPlan.score = score;
                    bestPlan.cueBallPlacement = cuePos;
                    bestPlan.aimAngle = shot.angle;
                    bestPlan.power = shot.power;
                    bestPlan.spin = shot.spin;
                    bestPlan.expectedPocketedCount = res.solutionFound ? 1 : 0;
                }
            }
        }
    }

    return bestPlan;
}

double BreakPlanner::EvaluateBreakOutcome(const ShotResult& result, BreakStrategy strategy) {
    if (result.isScratch) return -10000.0;

    double score = 0.0;

    // Pocketing on the break awards major bonus
    if (result.solutionFound) {
        score += 500.0;
        if (result.eightBallPocketed) {
            // In 8-ball rules, pocketing the 8-ball on the break can be a win or re-rack
            score += 200.0;
        }
    }

    // Measure total kinetic energy dispersion / path movement of balls
    for (const auto& path : result.paths) {
        if (path.size() > 10) {
            score += path.size() * 2.0; // Ball moved significantly
        }
    }

    return score;
}

} // namespace pool

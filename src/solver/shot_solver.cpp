//
//  shot_solver.cpp
//  Reconstructed from libloader (AI Shot Solver Engine)
//
//  Implements Multi-Strategy Search for Winning Shot Trajectories
//

#include "shot_solver.h"
#include <chrono>
#include <cmath>

namespace pool {

ShotSolver::ShotSolver(const TableShape& table, const FrictionProperties& friction)
    : m_table(table), m_friction(friction), m_simulator(table, friction) {}

SolverOutput ShotSolver::Solve(const SolverQuery& query,
                               const std::function<bool()>& cancellationToken) {
    auto startTime = std::chrono::high_resolution_clock::now();
    SolverOutput output;

    if (query.balls.empty() || query.balls[0].state != BallState::OnTable) {
        output.status = SolverStatus::NoSolution;
        return output;
    }

    // 1. Try Direct Shots First (Fastest & Most Reliable)
    if (SolveDirect(query, output)) {
        output.status = SolverStatus::Solution;
        output.strategyUsed = ShotStrategy::Direct;
    }
    // 2. Try Single-Cushion Bank Shots
    else if (SolveBank(query, output)) {
        output.status = SolverStatus::Solution;
        output.strategyUsed = ShotStrategy::CushionWindow;
    }
    // 3. Try Kick Shots
    else if (SolveKick(query, output)) {
        output.status = SolverStatus::Solution;
        output.strategyUsed = ShotStrategy::CueCushionFallback;
    }
    // 4. Fallback to Brute-Force Parameter Search
    else if (SolveBrute(query, output)) {
        output.status = SolverStatus::Solution;
        output.strategyUsed = ShotStrategy::Brute;
    } else {
        output.status = SolverStatus::NoSolution;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    output.computeTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    return output;
}

bool ShotSolver::SolveDirect(const SolverQuery& query, SolverOutput& output) {
    const Vector2D cuePos = query.balls[0].physics.Position();
    const double ballRadius = query.balls[0].physics.radius;

    for (int32_t targetNum : query.targetBallNumbers) {
        auto it = std::find_if(query.balls.begin(), query.balls.end(),
                               [targetNum](const Ball& b) { return b.number == targetNum; });
        if (it == query.balls.end() || it->state != BallState::OnTable) continue;

        const Vector2D objPos = it->physics.Position();

        for (const auto& pocket : m_table.pockets) {
            if (query.nominatedPocket >= 0 && pocket.id != query.nominatedPocket) continue;

            // Vector from object ball to pocket
            Vector2D toPocket = (pocket.position - objPos).Normalized();

            // "Ghost ball" position where cue ball must strike object ball
            Vector2D ghostBallPos = objPos - (toPocket * (ballRadius * 2.0));

            // Aim angle from cue ball to ghost ball
            Vector2D cueToGhost = ghostBallPos - cuePos;
            double aimAngle = std::atan2(cueToGhost.y, cueToGhost.x);

            // Test various powers (e.g., 0.35, 0.65, 0.90)
            const double testPowers[] = { 0.40, 0.70, 0.95 };
            for (double power : testPowers) {
                ShotParameters shot;
                shot.angle = aimAngle;
                shot.power = power;
                shot.spin = {0.0, 0.0};

                output.evaluatedShotCount++;
                ShotResult res = m_simulator.SimulateShot(query.balls, shot, 600);

                if (res.solutionFound && !res.isScratch && res.pocketedBallNumber == targetNum) {
                    if (query.nominatedPocket < 0 || res.pocketId == query.nominatedPocket) {
                        output.bestShot = shot;
                        output.simulationResult = res;
                        output.score = 1000.0 - cueToGhost.Length();
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool ShotSolver::SolveBank(const SolverQuery& query, SolverOutput& output) {
    // Bank shot: object ball reflects off cushion into pocket
    // Uses mirror pocket reflection principle
    return false; // Reconstructed skeleton
}

bool ShotSolver::SolveKick(const SolverQuery& query, SolverOutput& output) {
    // Kick shot: cue ball reflects off cushion before hitting object ball
    return false; // Reconstructed skeleton
}

bool ShotSolver::SolveBrute(const SolverQuery& query, SolverOutput& output) {
    const Vector2D cuePos = query.balls[0].physics.Position();
    const double stepAngle = 0.02; // ~1 degree steps
    const double twoPi = 2.0 * M_PI;

    double bestScore = -1e9;
    bool found = false;

    for (double angle = 0.0; angle < twoPi; angle += stepAngle) {
        ShotParameters shot;
        shot.angle = angle;
        shot.power = 0.75;
        shot.spin = {0.0, 0.0};

        output.evaluatedShotCount++;
        ShotResult res = m_simulator.SimulateShot(query.balls, shot, 400);

        if (res.solutionFound && !res.isScratch) {
            double score = EvaluateShotQuality(res, query);
            if (score > bestScore) {
                bestScore = score;
                output.bestShot = shot;
                output.simulationResult = res;
                found = true;
            }
        }
    }

    return found;
}

double ShotSolver::EvaluateShotQuality(const ShotResult& result, const SolverQuery& query) {
    if (result.isScratch) return -10000.0;

    double score = 100.0;
    if (query.isEightBall) {
        if (result.eightBallPocketed) score += 500.0;
    } else {
        auto it = std::find(query.targetBallNumbers.begin(), query.targetBallNumbers.end(), result.pocketedBallNumber);
        if (it != query.targetBallNumbers.end()) {
            score += 300.0;
            if (query.nominatedPocket >= 0 && result.pocketId == query.nominatedPocket) {
                score += 200.0;
            }
        }
    }
    return score;
}

} // namespace pool

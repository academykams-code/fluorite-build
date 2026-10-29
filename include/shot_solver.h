//
//  shot_solver.h
//  Reconstructed from libloader (AI Shot Solver Engine)
//
//  Calculates Winning Shot Solutions, Bank Shots, Kick Shots, and Auto-Play Trajectories
//

#pragma once

#include "pool_physics.h"
#include <string>
#include <functional>

namespace pool {

enum class ShotStrategy : int32_t {
    Direct = 0,             // Direct pot
    CushionWindow = 1,      // Single-cushion bank shot
    CushionFullCircle = 2,  // Multi-cushion bank shot
    CueCushionFallback = 3, // Kick shot (cue ball banks before hitting object ball)
    Brute = 4,              // Coarse-to-fine parameter sweep
    BreakBrute = 5          // Break shot optimization
};

enum class SolverStatus : int32_t {
    Solution = 0,
    NoSolution = 1,
    Timeout = 2,
    Cancelled = 3,
    AcceptedNonSolution = 4,
    MatchedInvalidResult = 5
};

struct SolverQuery {
    std::vector<Ball> balls;
    std::vector<int32_t> targetBallNumbers;
    int32_t nominatedPocket{-1};
    bool isEightBall{false};
    bool requireNomination{false};
    bool requireCushion{false};
    double maxComputeTimeSeconds{0.5};
};

struct SolverOutput {
    SolverStatus status{SolverStatus::NoSolution};
    ShotStrategy strategyUsed{ShotStrategy::Direct};
    ShotParameters bestShot;
    ShotResult simulationResult;
    double score{0.0};
    int32_t evaluatedShotCount{0};
    double computeTimeMs{0.0};
};

class ShotSolver {
public:
    ShotSolver(const TableShape& table, const FrictionProperties& friction);

    SolverOutput Solve(const SolverQuery& query,
                       const std::function<bool()>& cancellationToken = nullptr);

private:
    TableShape m_table;
    FrictionProperties m_friction;
    TrajectorySimulator m_simulator;

    bool SolveDirect(const SolverQuery& query, SolverOutput& output);
    bool SolveBank(const SolverQuery& query, SolverOutput& output);
    bool SolveKick(const SolverQuery& query, SolverOutput& output);
    bool SolveBrute(const SolverQuery& query, SolverOutput& output);

    double EvaluateShotQuality(const ShotResult& result, const SolverQuery& query);
};

} // namespace pool

//
//  pool_physics.h
//  Reconstructed from libloader (__TEXT,__cstring & Resolver strings)
//
//  Miniclip 8 Ball Pool Physics Engine Data Structures & Trajectory Simulation
//

#pragma once

#include <cmath>
#include <vector>
#include <cstdint>

namespace pool {

struct Vector2D {
    double x{0.0};
    double y{0.0};

    constexpr Vector2D() = default;
    constexpr Vector2D(double px, double py) : x(px), y(py) {}

    Vector2D operator+(const Vector2D& o) const { return {x + o.x, y + o.y}; }
    Vector2D operator-(const Vector2D& o) const { return {x - o.x, y - o.y}; }
    Vector2D operator*(double s) const { return {x * s, y * s}; }
    Vector2D operator/(double s) const { return {x / s, y / s}; }

    double Length() const { return std::sqrt(x * x + y * y); }
    double LengthSq() const { return x * x + y * y; }

    Vector2D Normalized() const {
        double len = Length();
        if (len < 1e-9) return {0.0, 0.0};
        return {x / len, y / len};
    }

    double Dot(const Vector2D& o) const { return x * o.x + y * o.y; }
    double Cross(const Vector2D& o) const { return x * o.y - y * o.x; }
};

struct Vector3D {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    constexpr Vector3D() = default;
    constexpr Vector3D(double px, double py, double pz) : x(px), y(py), z(pz) {}
};

/**
 * BallPhysics: Exact state layout mapped by Resolver
 */
struct BallPhysics {
    double x{0.0};
    double y{0.0};
    double vx{0.0};
    double vy{0.0};
    double radius{10.0}; // Default 8BP ball radius
    double spinX{0.0};
    double spinY{0.0};
    double spinZ{0.0};

    Vector2D Position() const { return {x, y}; }
    Vector2D Velocity() const { return {vx, vy}; }
    void SetPosition(const Vector2D& pos) { x = pos.x; y = pos.y; }
    void SetVelocity(const Vector2D& vel) { vx = vel.x; vy = vel.y; }
};

/**
 * Friction: Table and cushion friction constants mapped by Resolver
 */
struct FrictionProperties {
    double sliding{0.2};
    double rolling{0.015};
    double timeEquilibrium{0.1};
    double velReductionSliding{0.98};
    double velReductionRolling{0.995};
    double spinningFriction{0.95};
    double cushionRestitution{0.75};
    double cushionSpinRatio{0.6};
};

enum class BallState : int32_t {
    OnTable = 0,
    InPocket = 1,
    Scratch = 2
};

enum class BallClassification : int32_t {
    Cue = 0,
    Solid = 1,
    Stripe = 2,
    EightBall = 3
};

struct Ball {
    int32_t number{0};
    BallClassification classification{BallClassification::Cue};
    BallState state{BallState::OnTable};
    bool movable{true};
    bool shouldGoOnRailWhenPocketed{false};
    BallPhysics physics;
};

struct Pocket {
    Vector2D position;
    double radius{18.0};
    int32_t id{0};
};

struct CushionSegment {
    Vector2D p1;
    Vector2D p2;
    Vector2D normal;
};

struct TableShape {
    double minX{0.0};
    double maxX{800.0};
    double minY{0.0};
    double maxY{400.0};
    std::vector<Pocket> pockets;
    std::vector<CushionSegment> cushions;
};

struct ShotParameters {
    double angle{0.0};      // Radians
    double power{0.0};      // 0.0 to 1.0 (or impulse)
    Vector2D spin{0.0, 0.0}; // English / Cue-ball strike offset (-1.0 to 1.0)
};

struct TrajectoryPoint {
    Vector2D position;
    Vector2D velocity;
    int32_t eventType{0}; // 0 = step, 1 = cushion bounce, 2 = ball collision, 3 = pocketed
    int32_t targetBallNumber{-1};
};

struct ShotResult {
    bool solutionFound{false};
    int32_t pocketedBallNumber{-1};
    int32_t pocketId{-1};
    bool isScratch{false};
    bool eightBallPocketed{false};
    std::vector<std::vector<TrajectoryPoint>> paths; // Per-ball trajectory steps
};

/**
 * TrajectorySimulator: High-speed forward trajectory prediction
 */
class TrajectorySimulator {
public:
    TrajectorySimulator(const TableShape& table, const FrictionProperties& friction);

    // Runs forward simulation for a given shot candidate
    ShotResult SimulateShot(const std::vector<Ball>& initialBalls, const ShotParameters& shot, int32_t maxTicks = 1200);

private:
    TableShape m_table;
    FrictionProperties m_friction;

    bool StepSimulation(std::vector<Ball>& balls, double dt, ShotResult& result);
    void HandleBallCushionCollisions(std::vector<Ball>& balls, ShotResult& result);
    void HandleBallBallCollisions(std::vector<Ball>& balls, ShotResult& result);
    void CheckPocketing(std::vector<Ball>& balls, ShotResult& result);
};

} // namespace pool

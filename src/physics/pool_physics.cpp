//
//  pool_physics.cpp
//  Reconstructed from libloader (Physics & Forward Trajectory Engine)
//
//  Implements realistic 8 Ball Pool collision, friction, spin, and trajectory simulation
//

#include "pool_physics.h"
#include <algorithm>

namespace pool {

TrajectorySimulator::TrajectorySimulator(const TableShape& table, const FrictionProperties& friction)
    : m_table(table), m_friction(friction) {}

ShotResult TrajectorySimulator::SimulateShot(const std::vector<Ball>& initialBalls,
                                             const ShotParameters& shot,
                                             int32_t maxTicks) {
    ShotResult result;
    std::vector<Ball> balls = initialBalls;
    result.paths.resize(balls.size());

    // Record initial positions
    for (size_t i = 0; i < balls.size(); ++i) {
        result.paths[i].push_back({balls[i].physics.Position(), balls[i].physics.Velocity(), 0, -1});
    }

    // Apply cue impulse to ball 0 (cue ball)
    if (!balls.empty() && balls[0].state == BallState::OnTable) {
        double maxImpulse = 2500.0; // Scaled impulse
        double speed = shot.power * maxImpulse;
        balls[0].physics.vx = std::cos(shot.angle) * speed;
        balls[0].physics.vy = std::sin(shot.angle) * speed;

        // Apply spin: english translates into angular velocity
        balls[0].physics.spinX = shot.spin.x * 50.0;
        balls[0].physics.spinY = shot.spin.y * 50.0;
        balls[0].physics.spinZ = shot.spin.x * -30.0; // Side spin creates swerve / z-spin
    }

    const double dt = 0.008; // ~120 Hz internal physics tick
    for (int32_t tick = 0; tick < maxTicks; ++tick) {
        bool anyMoving = StepSimulation(balls, dt, result);

        // Record path sample every 2 ticks
        if (tick % 2 == 0) {
            for (size_t i = 0; i < balls.size(); ++i) {
                if (balls[i].state == BallState::OnTable) {
                    result.paths[i].push_back({balls[i].physics.Position(), balls[i].physics.Velocity(), 0, -1});
                }
            }
        }

        if (!anyMoving) {
            break;
        }
    }

    return result;
}

bool TrajectorySimulator::StepSimulation(std::vector<Ball>& balls, double dt, ShotResult& result) {
    bool anyMoving = false;
    const double velThreshold = 0.5;

    // 1. Update ball positions and apply friction
    for (auto& ball : balls) {
        if (ball.state != BallState::OnTable) continue;

        double speed = ball.physics.Velocity().Length();
        if (speed > velThreshold) {
            anyMoving = true;

            // Integration
            ball.physics.x += ball.physics.vx * dt;
            ball.physics.y += ball.physics.vy * dt;

            // Friction decay
            // Transition from sliding to rolling friction model
            double decay = m_friction.velReductionRolling;
            ball.physics.vx *= decay;
            ball.physics.vy *= decay;

            // Spin decay
            ball.physics.spinX *= m_friction.spinningFriction;
            ball.physics.spinY *= m_friction.spinningFriction;
            ball.physics.spinZ *= m_friction.spinningFriction;

            // Apply spin curve / throw to linear velocity
            ball.physics.vx += ball.physics.spinX * 0.05 * dt;
            ball.physics.vy += ball.physics.spinY * 0.05 * dt;
        } else {
            ball.physics.vx = 0.0;
            ball.physics.vy = 0.0;
            ball.physics.spinX = 0.0;
            ball.physics.spinY = 0.0;
            ball.physics.spinZ = 0.0;
        }
    }

    // 2. Cushion collisions
    HandleBallCushionCollisions(balls, result);

    // 3. Ball-ball collisions
    HandleBallBallCollisions(balls, result);

    // 4. Pocket detection
    CheckPocketing(balls, result);

    return anyMoving;
}

void TrajectorySimulator::HandleBallCushionCollisions(std::vector<Ball>& balls, ShotResult& result) {
    for (size_t i = 0; i < balls.size(); ++i) {
        auto& ball = balls[i];
        if (ball.state != BallState::OnTable) continue;

        // Check against table borders / cushion segments
        if (ball.physics.x - ball.physics.radius < m_table.minX) {
            ball.physics.x = m_table.minX + ball.physics.radius;
            ball.physics.vx = -ball.physics.vx * m_friction.cushionRestitution;
            ball.physics.vy += ball.physics.spinZ * m_friction.cushionSpinRatio;
            result.paths[i].push_back({ball.physics.Position(), ball.physics.Velocity(), 1, -1});
        } else if (ball.physics.x + ball.physics.radius > m_table.maxX) {
            ball.physics.x = m_table.maxX - ball.physics.radius;
            ball.physics.vx = -ball.physics.vx * m_friction.cushionRestitution;
            ball.physics.vy -= ball.physics.spinZ * m_friction.cushionSpinRatio;
            result.paths[i].push_back({ball.physics.Position(), ball.physics.Velocity(), 1, -1});
        }

        if (ball.physics.y - ball.physics.radius < m_table.minY) {
            ball.physics.y = m_table.minY + ball.physics.radius;
            ball.physics.vy = -ball.physics.vy * m_friction.cushionRestitution;
            ball.physics.vx -= ball.physics.spinZ * m_friction.cushionSpinRatio;
            result.paths[i].push_back({ball.physics.Position(), ball.physics.Velocity(), 1, -1});
        } else if (ball.physics.y + ball.physics.radius > m_table.maxY) {
            ball.physics.y = m_table.maxY - ball.physics.radius;
            ball.physics.vy = -ball.physics.vy * m_friction.cushionRestitution;
            ball.physics.vx += ball.physics.spinZ * m_friction.cushionSpinRatio;
            result.paths[i].push_back({ball.physics.Position(), ball.physics.Velocity(), 1, -1});
        }
    }
}

void TrajectorySimulator::HandleBallBallCollisions(std::vector<Ball>& balls, ShotResult& result) {
    const size_t count = balls.size();
    for (size_t i = 0; i < count; ++i) {
        if (balls[i].state != BallState::OnTable) continue;

        for (size_t j = i + 1; j < count; ++j) {
            if (balls[j].state != BallState::OnTable) continue;

            Vector2D delta = balls[j].physics.Position() - balls[i].physics.Position();
            double distSq = delta.LengthSq();
            double minDist = balls[i].physics.radius + balls[j].physics.radius;

            if (distSq < minDist * minDist && distSq > 1e-6) {
                double dist = std::sqrt(distSq);
                Vector2D normal = delta / dist;

                // Relative velocity along collision normal
                Vector2D relVel = balls[i].physics.Velocity() - balls[j].physics.Velocity();
                double velAlongNormal = relVel.Dot(normal);

                if (velAlongNormal > 0.0) {
                    // Elastic impact impulse (equal mass balls)
                    double impulse = velAlongNormal;
                    balls[i].physics.vx -= normal.x * impulse;
                    balls[i].physics.vy -= normal.y * impulse;
                    balls[j].physics.vx += normal.x * impulse;
                    balls[j].physics.vy += normal.y * impulse;

                    // Positional separation to prevent overlapping
                    double overlap = 0.5 * (minDist - dist);
                    balls[i].physics.x -= normal.x * overlap;
                    balls[i].physics.y -= normal.y * overlap;
                    balls[j].physics.x += normal.x * overlap;
                    balls[j].physics.y += normal.y * overlap;

                    // Record collision event
                    result.paths[i].push_back({balls[i].physics.Position(), balls[i].physics.Velocity(), 2, balls[j].number});
                    result.paths[j].push_back({balls[j].physics.Position(), balls[j].physics.Velocity(), 2, balls[i].number});
                }
            }
        }
    }
}

void TrajectorySimulator::CheckPocketing(std::vector<Ball>& balls, ShotResult& result) {
    for (auto& ball : balls) {
        if (ball.state != BallState::OnTable) continue;

        for (const auto& pocket : m_table.pockets) {
            Vector2D toPocket = pocket.position - ball.physics.Position();
            if (toPocket.Length() < pocket.radius) {
                // Ball entered pocket
                ball.state = BallState::InPocket;
                ball.physics.vx = 0.0;
                ball.physics.vy = 0.0;

                if (ball.number == 0) {
                    result.isScratch = true;
                } else if (ball.number == 8) {
                    result.eightBallPocketed = true;
                    result.pocketedBallNumber = 8;
                    result.pocketId = pocket.id;
                    result.solutionFound = true;
                } else {
                    result.pocketedBallNumber = ball.number;
                    result.pocketId = pocket.id;
                    result.solutionFound = true;
                }
                break;
            }
        }
    }
}

} // namespace pool

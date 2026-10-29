//
//  touch_calibrator.h
//  Reconstructed from libloader (Touch Calibration & Coordinate Projection)
//
//  Maps between UIKit Screen Points, Cocos2d-x World Space, and 8BP Table Coordinates
//

#pragma once

#include "pool_physics.h"
#include <CoreGraphics/CoreGraphics.h>

namespace pool {

struct ScreenBounds {
    double screenWidth{0.0};
    double screenHeight{0.0};
    double safeAreaInsetsLeft{0.0};
    double safeAreaInsetsRight{0.0};
    double safeAreaInsetsTop{0.0};
    double safeAreaInsetsBottom{0.0};
};

struct TableViewport {
    Vector2D visualTableOrigin{0.0, 0.0};
    double tableWidthInPoints{0.0};
    double tableHeightInPoints{0.0};
    double scaleFactor{1.0};
};

class TouchCalibrator {
public:
    static TouchCalibrator& Instance();

    void UpdateCalibration(const ScreenBounds& screen, const TableViewport& viewport);
    bool IsCalibrated() const { return m_calibrated; }

    // Coordinate conversions
    Vector2D TableToScreen(const Vector2D& tablePos) const;
    Vector2D ScreenToTable(const Vector2D& screenPos) const;

    // English spin control mapping
    Vector2D EnglishOffsetToScreen(const Vector2D& spinOffset, const Vector2D& hudCenter, double hudRadius) const;
    Vector2D ScreenToEnglishOffset(const Vector2D& screenTouch, const Vector2D& hudCenter, double hudRadius) const;

    // Touch event generation for automated shot firing
    CGPoint CalculateCueTouchPoint(double aimAngle, double cueDistance) const;
    CGPoint CalculatePowerTouchPoint(double powerRatio) const;

private:
    TouchCalibrator() = default;
    bool m_calibrated{false};
    ScreenBounds m_screen;
    TableViewport m_viewport;
};

} // namespace pool

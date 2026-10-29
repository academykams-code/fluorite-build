//
//  touch_calibrator.cpp
//  Reconstructed from libloader (Touch Calibration & Coordinate Projection)
//
//  Maps between UIKit Screen Points, Cocos2d-x World Space, and 8BP Table Coordinates
//

#include "touch_calibrator.h"
#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#import <objc/message.h>
#include <cstdio>
#include <algorithm>

namespace pool {

TouchCalibrator& TouchCalibrator::Instance() {
    static TouchCalibrator s_instance;
    return s_instance;
}

void TouchCalibrator::UpdateCalibration(const ScreenBounds& screen, const TableViewport& viewport) {
    m_screen = screen;
    m_viewport = viewport;
    m_calibrated = (viewport.tableWidthInPoints > 0.0 && viewport.tableHeightInPoints > 0.0);

    if (m_calibrated) {
        printf("[TouchCalibrator] Calibrated: Screen(%.0fx%.0f) TableOrigin(%.1f, %.1f) Scale(%.4f)\n",
               screen.screenWidth, screen.screenHeight,
               viewport.visualTableOrigin.x, viewport.visualTableOrigin.y,
               viewport.scaleFactor);
    }
}

Vector2D TouchCalibrator::TableToScreen(const Vector2D& tablePos) const {
    if (!m_calibrated) return tablePos;
    // Apply visual origin offset and scale factor
    double screenX = m_viewport.visualTableOrigin.x + (tablePos.x * m_viewport.scaleFactor);
    double screenY = m_viewport.visualTableOrigin.y + (tablePos.y * m_viewport.scaleFactor);
    return {screenX, screenY};
}

Vector2D TouchCalibrator::ScreenToTable(const Vector2D& screenPos) const {
    if (!m_calibrated || m_viewport.scaleFactor <= 0.0) return screenPos;
    double tableX = (screenPos.x - m_viewport.visualTableOrigin.x) / m_viewport.scaleFactor;
    double tableY = (screenPos.y - m_viewport.visualTableOrigin.y) / m_viewport.scaleFactor;
    return {tableX, tableY};
}

Vector2D TouchCalibrator::EnglishOffsetToScreen(const Vector2D& spinOffset,
                                               const Vector2D& hudCenter,
                                               double hudRadius) const {
    // SpinOffset is in range [-1.0, 1.0]
    double clampedX = std::clamp(spinOffset.x, -1.0, 1.0);
    double clampedY = std::clamp(spinOffset.y, -1.0, 1.0);
    return {hudCenter.x + clampedX * hudRadius, hudCenter.y - clampedY * hudRadius};
}

Vector2D TouchCalibrator::ScreenToEnglishOffset(const Vector2D& screenTouch,
                                               const Vector2D& hudCenter,
                                               double hudRadius) const {
    if (hudRadius <= 0.0) return {0.0, 0.0};
    double dx = (screenTouch.x - hudCenter.x) / hudRadius;
    double dy = -(screenTouch.y - hudCenter.y) / hudRadius; // Invert Y
    double len = std::sqrt(dx * dx + dy * dy);
    if (len > 1.0) {
        dx /= len;
        dy /= len;
    }
    return {dx, dy};
}

CGPoint TouchCalibrator::CalculateCueTouchPoint(double aimAngle, double cueDistance) const {
    // Vector pointing back along the cue stick line
    double touchX = std::cos(aimAngle + M_PI) * cueDistance;
    double touchY = std::sin(aimAngle + M_PI) * cueDistance;
    return CGPointMake(touchX, touchY);
}

CGPoint TouchCalibrator::CalculatePowerTouchPoint(double powerRatio) const {
    // In 8 Ball Pool, the power meter slider runs vertically on the left side of the screen
    double clampedPower = std::clamp(powerRatio, 0.0, 1.0);
    double meterX = 40.0 + m_screen.safeAreaInsetsLeft;
    double meterTopY = m_screen.screenHeight * 0.25;
    double meterBottomY = m_screen.screenHeight * 0.85;
    double touchY = meterTopY + clampedPower * (meterBottomY - meterTopY);
    return CGPointMake(meterX, touchY);
}

} // namespace pool

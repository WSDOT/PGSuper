#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

// Geometry-only validation shared with the portable regression checks.
namespace xbrScallop
{
struct Point { double x, y; };

enum class ArcError { None, InvalidChord, AboveTop, FoldedArc };

inline ArcError ValidateArc(Point p0, Point p1, double radius, const std::vector<Point>& top)
{
   constexpr double tolerance = 1.0e-8;
   const double dx = p1.x - p0.x, dy = p1.y - p0.y;
   const double chord = std::hypot(dx, dy);
   if (!std::isfinite(radius) || radius <= 0 || !std::isfinite(chord) || chord > 2 * radius)
      return ArcError::InvalidChord;
   if (chord < tolerance)
      return ArcError::None;
   if (dx <= 0)
      return ArcError::FoldedArc;

   const double h = std::sqrt((std::max)(0.0, radius * radius - chord * chord / 4));
   // The lower center defines the upper minor arc for left-to-right endpoints.
   const double cx = (p0.x + p1.x) / 2 + h * dy / chord;
   const double cy = (p0.y + p1.y) / 2 - h * dx / chord;
   const double a0 = std::atan2(p0.y - cy, p0.x - cx);
   const double a1 = std::atan2(p1.y - cy, p1.x - cx);
   const double sweep = std::atan2(std::sin(a1 - a0), std::cos(a1 - a0));
   auto onArc = [=](double a)
   {
      const double delta = std::atan2(std::sin(a - a0), std::cos(a - a0));
      return delta >= sweep - tolerance && delta <= tolerance;
   };
   // A bottom surface must be single valued in x. Tilted, tight arcs can
   // turn back past an endpoint even when their chord fits the diameter.
   for (int i = 0; i <= 24; ++i)
   {
      const double a = a0 + sweep * i / 24;
      if (-std::sin(a) * sweep < -tolerance)
         return ArcError::FoldedArc;
   }

   for (size_t i = 1; i < top.size(); ++i)
   {
      const auto t0 = top[i - 1], t1 = top[i];
      const double left = (std::max)(p0.x, t0.x), right = (std::min)(p1.x, t1.x);
      if (right < left || t1.x <= t0.x)
         continue;
      const double slope = (t1.y - t0.y) / (t1.x - t0.x);
      auto above = [=](double x, double y)
      {
         return y > t0.y + slope * (x - t0.x) + tolerance;
      };
      // Endpoints of each top-profile segment and the stationary point
      // of (arc elevation - top elevation) cover the maximum clearance.
      for (double x : {left, right})
      {
         const double y = cy + std::sqrt((std::max)(0.0, radius * radius - (x - cx) * (x - cx)));
         if (above(x, y))
            return ArcError::AboveTop;
      }
      const double peakAngle = std::atan2(1.0, -slope);
      const double peakX = cx + radius * std::cos(peakAngle);
      if (onArc(peakAngle) && peakX >= left && peakX <= right &&
          above(peakX, cy + radius * std::sin(peakAngle)))
         return ArcError::AboveTop;
   }
   return ArcError::None;
}
}

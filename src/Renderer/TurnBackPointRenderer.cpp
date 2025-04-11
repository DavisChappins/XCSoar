// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TurnBackPointRenderer.hpp"
#include "Look/MapLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Math/Angle.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Projection/WindowProjection.hpp"
#include "Geo/Math.hpp"
#include "Screen/Layout.hpp"
#include "Math/Screen.hpp"

void
TurnBackPointRenderer::Draw(Canvas &canvas,
                           const WindowProjection &projection,
                           [[maybe_unused]] const PixelPoint pos,
                           const NMEAInfo &basic,
                           const DerivedInfo &calculated) const noexcept
{
  // Check if we have a valid task and solution
  const TaskStats &task_stats = calculated.task_stats;
  if (!task_stats.task_valid)
    return;

  const ElementStat &total = task_stats.total;
  const GlideResult &solution = total.solution_remaining;
  
  // Only show TBP when we're above glide
  if (!solution.IsOk() || solution.altitude_difference <= 0)
    return;
  
  // Check if we have a valid track
  if (!basic.track_available)
    return;
  
  // Calculate the glide ratio
  if (solution.height_glide <= 0)
    return;
    
  const double glide_ratio = solution.vector.distance / solution.height_glide;
  
  // Calculate distance to the TBP
  const double distance_to_tbp = solution.altitude_difference * glide_ratio;
  
  // Find the position of the TBP
  GeoPoint tbp_location = FindLatitudeLongitude(basic.location, 
                                               basic.track, 
                                               distance_to_tbp);
  
  // Convert to screen coordinates
  auto tbp_screen = projection.GeoToScreenIfVisible(tbp_location);
  if (!tbp_screen)
    return;
  
  // Define triangle shape (pointing in direction of travel)
  BulkPixelPoint triangle[3];
  triangle[0].x = 0;
  triangle[0].y = -5;
  triangle[1].x = -4;
  triangle[1].y = 3;
  triangle[2].x = 4;
  triangle[2].y = 3;
  
  // Rotate triangle to align with track
  PolygonRotateShift(std::span<BulkPixelPoint>(triangle, 3), *tbp_screen,
                     basic.track - projection.GetScreenAngle());
  
  // Draw filled triangle
  canvas.Select(look.tbp_pen);
  canvas.Select(look.tbp_brush);
  canvas.DrawPolygon(triangle, 3);
}
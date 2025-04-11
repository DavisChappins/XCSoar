// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskProfile.hpp"
#include "RouteProfile.hpp" // Ensure this is included for Save(..., RoutePlannerConfig)
#include "Map.hpp"
#include "Keys.hpp"
#include "Engine/Task/TaskBehaviour.hpp" // Correct include path

namespace Profile {
  static void Load(const ProfileMap &map, GlideSettings &settings);
  static void Load(const ProfileMap &map, TaskStartMargins &settings);
  static void Load(const ProfileMap &map, SectorDefaults &settings);
  static void Load(const ProfileMap &map, StartConstraints &constraints);
  static void Load(const ProfileMap &map, FinishConstraints &constraints);
  static void Load(const ProfileMap &map, OrderedTaskSettings &settings);
};

void
Profile::Load(const ProfileMap &map, GlideSettings &settings)
{
  map.Get(ProfileKeys::PredictWindDrift, settings.predict_wind_drift);
}

void
Profile::Load(const ProfileMap &map, TaskStartMargins &settings)
{
  map.Get(ProfileKeys::StartMaxHeightMargin, settings.max_height_margin);
  map.Get(ProfileKeys::StartMaxSpeedMargin, settings.max_speed_margin);
}

void
Profile::Load(const ProfileMap &map, SectorDefaults &settings)
{
  map.GetEnum(ProfileKeys::StartType, settings.start_type);
  map.Get(ProfileKeys::StartRadius, settings.start_radius);
  map.GetEnum(ProfileKeys::TurnpointType, settings.turnpoint_type);
  map.Get(ProfileKeys::TurnpointRadius, settings.turnpoint_radius);
  map.GetEnum(ProfileKeys::FinishType, settings.finish_type);
  map.Get(ProfileKeys::FinishRadius, settings.finish_radius);
}

void
Profile::Load(const ProfileMap &map, StartConstraints &constraints)
{
  map.GetEnum(ProfileKeys::StartHeightRef, constraints.max_height_ref);
  map.Get(ProfileKeys::StartMaxHeight, constraints.max_height);
  map.Get(ProfileKeys::StartMaxSpeed, constraints.max_speed);
  map.Get(ProfileKeys::PEVStartWaitTime, constraints.pev_start_wait_time);
  map.Get(ProfileKeys::PEVStartWindow, constraints.pev_start_window);
}

void
Profile::Load(const ProfileMap &map, FinishConstraints &constraints)
{
  map.GetEnum(ProfileKeys::FinishHeightRef, constraints.min_height_ref);
  map.Get(ProfileKeys::FinishMinHeight, constraints.min_height);
}

void
Profile::Load(const ProfileMap &map, OrderedTaskSettings &settings)
{
  Load(map, settings.start_constraints);
  Load(map, settings.finish_constraints);
  map.Get(ProfileKeys::AATMinTime, settings.aat_min_time);
}

void
Profile::Load(const ProfileMap &map, TaskBehaviour &settings)
{
  Load(map, settings.glide);

  map.Get(ProfileKeys::AATTimeMargin, settings.optimise_targets_margin);
  map.Get(ProfileKeys::AutoMc, settings.auto_mc);
  map.Get(ProfileKeys::ArrivalRingAATEnabled, settings.arrival_ring_aat_enabled); // Remove default value argument
  map.GetEnum(ProfileKeys::AutoMcMode, settings.auto_mc_mode);

  unsigned Temp;
  if (map.Get(ProfileKeys::RiskGamma, Temp))
    settings.risk_gamma = Temp / 10.;

  if (map.Get(ProfileKeys::SafetyMacCready, Temp))
    settings.safety_mc = Temp / 10.;

  map.Get(ProfileKeys::SafetyAltitudeArrival, settings.safety_height_arrival);
  map.GetEnum(ProfileKeys::TaskType, settings.task_type_default);
  Load(map, settings.start_margins);

  Load(map, settings.sector_defaults);
  Load(map, settings.ordered_defaults);

  map.GetEnum(ProfileKeys::AbortTaskMode, settings.abort_task_mode);

  map.Get(ProfileKeys::TurnBackMarkerEnabled, settings.turn_back_marker_enabled);

  Load(map, settings.route_planner);
}

// Add corresponding Save function
namespace Profile {
  static void Save(ProfileMap &map, const GlideSettings &settings);
  static void Save(ProfileMap &map, const TaskStartMargins &settings);
  static void Save(ProfileMap &map, const SectorDefaults &settings);
  static void Save(ProfileMap &map, const StartConstraints &constraints);
  static void Save(ProfileMap &map, const FinishConstraints &constraints);
  static void Save(ProfileMap &map, const OrderedTaskSettings &settings);
  // Remove redundant static declaration for TaskBehaviour Save (already declared in .hpp)
  // Remove unnecessary forward declaration for RoutePlannerConfig Save
};

void
Profile::Save(ProfileMap &map, const GlideSettings &settings)
{
  map.Set(ProfileKeys::PredictWindDrift, settings.predict_wind_drift);
}

void
Profile::Save(ProfileMap &map, const TaskStartMargins &settings)
{
  map.Set(ProfileKeys::StartMaxHeightMargin, settings.max_height_margin);
  map.Set(ProfileKeys::StartMaxSpeedMargin, settings.max_speed_margin);
}

void
Profile::Save(ProfileMap &map, const SectorDefaults &settings)
{
  map.SetEnum(ProfileKeys::StartType, settings.start_type);
  map.Set(ProfileKeys::StartRadius, settings.start_radius);
  map.SetEnum(ProfileKeys::TurnpointType, settings.turnpoint_type);
  map.Set(ProfileKeys::TurnpointRadius, settings.turnpoint_radius);
  map.SetEnum(ProfileKeys::FinishType, settings.finish_type);
  map.Set(ProfileKeys::FinishRadius, settings.finish_radius);
}

void
Profile::Save(ProfileMap &map, const StartConstraints &constraints)
{
  map.SetEnum(ProfileKeys::StartHeightRef, constraints.max_height_ref);
  map.Set(ProfileKeys::StartMaxHeight, constraints.max_height);
  map.Set(ProfileKeys::StartMaxSpeed, constraints.max_speed);
  map.Set(ProfileKeys::PEVStartWaitTime, constraints.pev_start_wait_time);
  map.Set(ProfileKeys::PEVStartWindow, constraints.pev_start_window);
}

void
Profile::Save(ProfileMap &map, const FinishConstraints &constraints)
{
  map.SetEnum(ProfileKeys::FinishHeightRef, constraints.min_height_ref);
  map.Set(ProfileKeys::FinishMinHeight, constraints.min_height);
}

void
Profile::Save(ProfileMap &map, const OrderedTaskSettings &settings)
{
  Save(map, settings.start_constraints);
  Save(map, settings.finish_constraints);
  map.Set(ProfileKeys::AATMinTime, settings.aat_min_time);
}

void
Profile::Save(ProfileMap &map, const TaskBehaviour &settings)
{
  Save(map, settings.glide);

  map.Set(ProfileKeys::AATTimeMargin, settings.optimise_targets_margin);
  map.Set(ProfileKeys::AutoMc, settings.auto_mc);
  map.Set(ProfileKeys::ArrivalRingAATEnabled, settings.arrival_ring_aat_enabled); // Save the new setting
  map.SetEnum(ProfileKeys::AutoMcMode, settings.auto_mc_mode);

  map.Set(ProfileKeys::RiskGamma, (unsigned)(settings.risk_gamma * 10.));
  map.Set(ProfileKeys::SafetyMacCready, (unsigned)(settings.safety_mc * 10.));

  map.Set(ProfileKeys::SafetyAltitudeArrival, settings.safety_height_arrival);
  map.SetEnum(ProfileKeys::TaskType, settings.task_type_default);
  Save(map, settings.start_margins);

  Save(map, settings.sector_defaults);
  Save(map, settings.ordered_defaults);

  map.SetEnum(ProfileKeys::AbortTaskMode, settings.abort_task_mode);

  Save(map, settings.route_planner);
}

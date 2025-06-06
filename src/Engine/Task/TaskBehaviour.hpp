// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Ordered/Settings.hpp"
#include "Task/Factory/TaskPointFactoryType.hpp"
#include "Engine/Route/Config.hpp"
#include "GlideSolvers/GlideSettings.hpp"
#include "Task/Factory/TaskFactoryType.hpp"

#include <type_traits>

struct AircraftState;

enum class AbortTaskMode: uint8_t {
  SIMPLE,
  TASK,
  HOME,
};

/**
 * variables set user preference defaults for new task and
 * new turpoints created by the task factories
 */
struct SectorDefaults
{
  /** default start type for new tasks */
  TaskPointFactoryType start_type;
  /** default start radius or line length for new tasks */
  double start_radius;
  /** default intermediate type for new tasks */
  TaskPointFactoryType turnpoint_type;
  /** default intermediate point radius for new tasks */
  double turnpoint_radius;
  /** default finish type for new tasks */
  TaskPointFactoryType finish_type;
  /** default finish radius or line length for new tasks */
  double finish_radius;

  void SetDefaults();
};

struct TaskStartMargins {
  /** Margin in maximum ground speed (m/s) allowed in start sector */
  double max_speed_margin;

  /** Margin in maximum height (m) allowed in start sector */
  unsigned max_height_margin;

  void SetDefaults();
};

/**
 *  Class defining options for task system.
 *  Typical uses might be default values, and simple aspects of task behaviour.
 */
struct TaskBehaviour {
  GlideSettings glide;

  /**
   * Option to enable positionining of AAT targets to achieve
   * desired AAT minimum task time
   */
  bool optimise_targets_range;
  /** Option to enable positioning of AAT targets at optimal point on isoline */
  bool optimise_targets_bearing;
  /** Seconds additional to min time to optimise for */
  std::chrono::duration<unsigned> optimise_targets_margin;
  /** Option to enable calculation and setting of auto MacCready */
  bool auto_mc;
  /** Option to enable drawing the AAT arrival time ring */
  bool arrival_ring_aat_enabled;

  /** Enumeration of auto MC modes */
  enum class AutoMCMode: uint8_t {
    /** Final glide only */
    FINALGLIDE = 0,
    /** Climb average */
    CLIMBAVERAGE,
    /** Final glide if above FG, else climb average */
    BOTH
  };

  /** Options for auto MC calculations */
  AutoMCMode auto_mc_mode;
  
  /** Option to enable calculation of cruise efficiency */
  bool calc_cruise_efficiency;
  /** Option to enable calculation of effective mc */
  bool calc_effective_mc;
  /** Option to enable calculation of required sink rate for final glide */
  bool calc_glide_required;
  /** Option to enable Goto tasks for non-landable waypoints */
  bool goto_nonlandable;

  /** Compensation factor for risk at low altitude */
  double risk_gamma;

  /** Safety MacCready value (m/s) used by abort task */
  double safety_mc;

  /** Minimum height above terrain for arrival height at landable waypoint (m) */
  double safety_height_arrival;

  /** Default task type to use for new tasks */
  TaskFactoryType task_type_default;

  TaskStartMargins start_margins;

  /** Default sector info for new ordered task */
  SectorDefaults sector_defaults;

  /** Defaults for ordered task */
  OrderedTaskSettings ordered_defaults;

  /**
   * How should the Abort/Alternate task work like:
   * AbortTaskMode::SIMPLE: sort only by arrival height and wp type
   * AbortTaskMode::TASK: sort also by deflection from current turnpoint
   * AbortTaskMode::HOME: sort also by deflection from home
   */
  AbortTaskMode abort_task_mode;

  /** Route and reach planning */
  RoutePlannerConfig route_planner;

  /** Show a marker indicating the point of no return */
  bool turn_back_marker_enabled;

  void SetDefaults();

  /**
   * Convenience function (used primarily for testing) to disable
   * all expensive task behaviour functions.
   */
  void DisableAll();

  bool IsAutoMCFinalGlideEnabled() const {
    return auto_mc && auto_mc_mode != AutoMCMode::CLIMBAVERAGE;
  }

  bool IsAutoMCCruiseEnabled() const {
    return auto_mc && auto_mc_mode != AutoMCMode::FINALGLIDE;
  }
};

static_assert(std::is_trivial<TaskBehaviour>::value, "type is not trivial");

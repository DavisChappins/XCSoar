// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AnalyseFlight.hpp"
#include "DebugReplay.hpp"
#include "Engine/Trace/Trace.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Contest/ContestManager.hpp"
#include "Math/Angle.hpp"
#include "time/BrokenDateTime.hpp"
#include "Computer/CirclingComputer.hpp"
#include "Computer/Wind/Computer.hpp"
#include "Computer/Settings.hpp"
#include "Computer/AutoQNH.hpp"
#include "FlightPhaseDetector.hpp"

#include <limits>

using namespace std::chrono;

void
Run(DebugReplay &replay, FlightPhaseDetector &flight_phase_detector,
    WindList &wind_list,
    const BrokenDateTime &takeoff_time,
    const BrokenDateTime &scoring_start_time,
    const BrokenDateTime &scoring_end_time,
    const BrokenDateTime &landing_time,
    Trace &full_trace, Trace &triangle_trace, Trace &sprint_trace,
    ComputerSettings &computer_settings)
{
  GeoPoint last_location = GeoPoint::Invalid();
  constexpr Angle max_longitude_change = Angle::Degrees(30);
  constexpr Angle max_latitude_change = Angle::Degrees(1);

  CirclingSettings circling_settings;
  circling_settings.SetDefaults();
  CirclingComputer circling_computer;
  circling_computer.Reset();

  GlidePolar glide_polar(0);

  WindSettings wind_settings;
  wind_settings.SetDefaults();

  WindComputer wind_computer;
  wind_computer.Reset();

  Validity last_wind;
  last_wind.Clear();

  const Waypoints waypoints;
  AutoQNH auto_qnh(5);
  auto_qnh.Reset();

  const auto takeoff_tp = takeoff_time.ToTimePoint();
  const auto landing_tp = landing_time.ToTimePoint();

  auto scoring_start_tp = std::chrono::system_clock::time_point::min();
  auto scoring_end_tp = std::chrono::system_clock::time_point::max();

  if (scoring_start_time.IsPlausible())
    scoring_start_tp = scoring_start_time.ToTimePoint();

  if (scoring_end_time.IsPlausible())
    scoring_end_tp = scoring_end_time.ToTimePoint();

  while (replay.Next()) {
    const MoreData &basic = replay.Basic();
    const auto date_time_utc = basic.date_time_utc.ToTimePoint();

    if (date_time_utc < takeoff_tp)
      continue;

    if (date_time_utc > landing_tp)
      break;

    circling_computer.TurnRate(replay.SetCalculated(),
                               replay.Basic(),
                               replay.Calculated().flight);
    circling_computer.Turning(replay.SetCalculated(),
                              replay.Basic(),
                              replay.Calculated().flight,
                              circling_settings);

    flight_phase_detector.Update(replay.Basic(), replay.Calculated());

    wind_computer.Compute(wind_settings, glide_polar, basic,
                          replay.SetCalculated());

    if (replay.Calculated().estimated_wind_available.Modified(last_wind)) {
      wind_list.push_back(WindListItem(basic.date_time_utc, basic.gps_altitude,
                                       replay.Calculated().estimated_wind));
    }

    last_wind = replay.Calculated().estimated_wind_available;

    auto_qnh.Process(basic, replay.SetCalculated(), computer_settings, waypoints);

    if (!computer_settings.pressure_available && replay.Calculated().pressure_available) {
        computer_settings.pressure = replay.Calculated().pressure;
        computer_settings.pressure_available = replay.Calculated().pressure_available;
    }

    if (!basic.time_available || !basic.location_available ||
        !basic.NavAltitudeAvailable())
      continue;

    if (last_location.IsValid() &&
        ((last_location.latitude - basic.location.latitude).Absolute() > max_latitude_change ||
         (last_location.longitude - basic.location.longitude).Absolute() > max_longitude_change))
      /* there was an implausible warp, which is usually triggered by
         an invalid point declared "valid" by a bugged logger; if that
         happens, we stop the analysis, because the IGC file is
         obviously broken */
      break;

    last_location = basic.location;

    if (date_time_utc >= scoring_start_tp &&
        date_time_utc <= scoring_end_tp) {
      const TracePoint point(basic);
      full_trace.push_back(point);
      triangle_trace.push_back(point);
      sprint_trace.push_back(point);
    }
  }

  flight_phase_detector.Finish();
}

ContestStatistics
SolveContest(Contest contest,
             Trace &full_trace, Trace &triangle_trace, Trace &sprint_trace,
             const unsigned max_iterations, const unsigned max_tree_size)
{
  ContestManager manager(contest, full_trace, triangle_trace, sprint_trace);
  manager.SolveExhaustive(max_iterations, max_tree_size);
  return manager.GetStats();
}

void AnalyseFlight(DebugReplay &replay,
             const BrokenDateTime &takeoff_time,
             const BrokenDateTime &scoring_start_time,
             const BrokenDateTime &scoring_end_time,
             const BrokenDateTime &landing_time,
             ContestStatistics &olc_plus,
             ContestStatistics &dmst,
             PhaseList &phase_list,
             PhaseTotals &phase_totals,
             WindList &wind_list,
             ComputerSettings &computer_settings,
             const unsigned full_points,
             const unsigned triangle_points,
             const unsigned sprint_points,
             const unsigned max_iterations,
             const unsigned max_tree_size)
{
  Trace full_trace({}, Trace::null_time, full_points);
  Trace triangle_trace({}, Trace::null_time, triangle_points);
  Trace sprint_trace({}, minutes{120}, sprint_points);
  FlightPhaseDetector flight_phase_detector;

  Run(replay, flight_phase_detector, wind_list,
      takeoff_time, scoring_start_time, scoring_end_time, landing_time,
      full_trace, triangle_trace, sprint_trace,
      computer_settings);

  olc_plus = SolveContest(Contest::OLC_PLUS,
    full_trace, triangle_trace, sprint_trace,
    max_iterations, max_tree_size);
  dmst = SolveContest(Contest::DMST,
    full_trace, triangle_trace, sprint_trace,
    max_iterations, max_tree_size);

  phase_list = flight_phase_detector.GetPhases();
  phase_totals = flight_phase_detector.GetTotals();
}

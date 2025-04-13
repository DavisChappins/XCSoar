// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Id.hpp"
#include "Geo/GeoPoint.hpp"
#include "NMEA/Validity.hpp"
#include "time/Stamp.hpp"

#include <vector>

/**
 * @brief Holds state and calculated information about a detected thermal
 *        associated with a specific FLARM traffic object.
 */
struct TrafficThermalInfo {
  enum class State {
    IDLE,                 // Not currently considered circling
    POTENTIALLY_CIRCLING, // Meets some criteria, gathering more data
    CIRCLING,             // Confirmed circling
    RECENTLY_STOPPED      // Was circling, now stopped, keep displaying for a while
  };

  FlarmId id;                     // ID of the traffic object
  State current_state = State::IDLE;
  GeoPoint center_location;       // Estimated center of the thermal
  double average_climb_rate = 0.0; // Running average climb rate while circling
  TimeStamp last_update_time;     // Last time this info was updated (state change or circling update)
  TimeStamp circling_start_time;  // When circling state was entered

  // Internal state for detection logic
  std::vector<GeoPoint> position_history; // Limited history for area check
  double climb_rate_sum = 0.0;
  int climb_rate_samples = 0;
  // Could add bounding box tracking here as well

  void ResetDetectionState() {
    position_history.clear();
    climb_rate_sum = 0.0;
    climb_rate_samples = 0;
    average_climb_rate = 0.0;
    // Keep center_location and last_update_time for display after stopping
  }

  bool IsRelevantForDisplay(TimeStamp now, FloatDuration max_age) const {
    return current_state == State::CIRCLING ||
           (current_state == State::RECENTLY_STOPPED && (now - last_update_time).count() <= max_age.count());
  }
};
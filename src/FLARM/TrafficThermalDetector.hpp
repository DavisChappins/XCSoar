// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "TrafficThermal.hpp"
#include "Id.hpp"
#include "time/Stamp.hpp"
#include "time/FloatDuration.hxx" // For FloatDuration

#include <map>
#include <vector>
#include <memory> // For potential future use if needed

// Forward declarations
struct FlarmTraffic;

/**
 * @brief Detects and tracks circling behavior (thermaling) for FLARM traffic objects.
 */
class TrafficThermalDetector {
private:
  // Map storing the thermal state for each tracked FLARM ID
  std::map<FlarmId, TrafficThermalInfo> thermal_states;



  // Private helper methods for detection logic
  void UpdateCirclingState(TrafficThermalInfo& info, const FlarmTraffic& traffic, const FlarmTraffic* last_traffic, TimeStamp now);
  bool CheckAreaConstraint(const TrafficThermalInfo& info) const;
  void UpdateAverageClimb(TrafficThermalInfo& info, const FlarmTraffic& traffic);
  void UpdateCenterLocation(TrafficThermalInfo& info, const FlarmTraffic& traffic);


public:
  // Constants for detection logic (consider making these configurable later)
  static constexpr double MIN_TURN_RATE_DEG_S = 15.0;
  static constexpr double MAX_DIAMETER_M = 1000.0;
  static constexpr FloatDuration MIN_CIRCLING_DURATION = std::chrono::seconds{45};
  static constexpr FloatDuration MAX_RECENTLY_STOPPED_AGE = std::chrono::minutes{10};
  static constexpr size_t POSITION_HISTORY_SIZE = 10; // Store last N positions

  /**
   * @brief Updates the thermal state for a single traffic object based on its latest data.
   * @param traffic The current data for the traffic object.
   * @param last_traffic Pointer to the previous data for the same object (can be nullptr).
   * @param now The current timestamp.
   */
  void Update(const FlarmTraffic &traffic, const FlarmTraffic *last_traffic, TimeStamp now);

  /**
   * @brief Removes expired thermal information (older than MAX_RECENTLY_STOPPED_AGE).
   * @param now The current timestamp.
   */
  void Cleanup(TimeStamp now);

  /**
   * @brief Gets a list of thermal information objects relevant for display.
   *        Includes currently circling and recently stopped thermals.
   * @param now The current timestamp.
   * @return A map containing references to the relevant TrafficThermalInfo objects.
   */
  const std::map<FlarmId, TrafficThermalInfo>& GetDisplayableThermals() const;

};
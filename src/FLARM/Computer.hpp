// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Calculations.hpp"
#include "TrafficThermalDetector.hpp" // Added for thermal detection

struct FlarmData;
struct NMEAInfo;

class FlarmComputer {
  FlarmCalculations flarm_calculations;
  TrafficThermalDetector thermal_detector; // Added for thermal detection

public:
  /**
   * Calculates location, altitude, average climb speed and
   * looks up the callsign of each target
   */
  void Process(FlarmData &flarm, const FlarmData &last_flarm,
               const NMEAInfo &basic) noexcept;

 /**
  * @brief Gets the currently detected thermal information.
  */
 const std::map<FlarmId, TrafficThermalInfo>& GetDetectedThermals() const noexcept {
   return thermal_detector.GetDisplayableThermals();
 }
};

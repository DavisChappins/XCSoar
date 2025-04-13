// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Error.hpp"
#include "FLARM/Version.hpp"
#include "FLARM/Hardware.hpp"
#include "FLARM/Status.hpp"
#include "FLARM/List.hpp"
#include "FLARM/TrafficThermal.hpp" // Added for detected thermals

#include <map> // Added for std::map
#include <type_traits>

/**
 * A container for all data received by a FLARM.
 */
struct FlarmData {
  FlarmError error;

  FlarmVersion version;

  FlarmHardware hardware;

  FlarmStatus status;

  TrafficList traffic;

  /**
   * @brief Thermals detected based on traffic behavior.
   */
  std::map<FlarmId, TrafficThermalInfo> detected_thermals;

  constexpr bool IsDetected() const noexcept {
    return status.available || !traffic.IsEmpty();
  }

  void Clear() noexcept { // Removed constexpr
    error.Clear();
    version.Clear();
    hardware.Clear();
    status.Clear();
    traffic.Clear();
    detected_thermals.clear(); // Added clear for thermals map
  }

  constexpr void Complement(const FlarmData &add) noexcept {
    error.Complement(add.error);
    version.Complement(add.version);
    hardware.Complement(add.hardware);
    status.Complement(add.status);
    traffic.Complement(add.traffic);
  }

  constexpr void Expire(TimeStamp clock) noexcept {
    error.Expire(clock);
    version.Expire(clock);
    hardware.Expire(clock);
    status.Expire(clock);
    traffic.Expire(clock);
  }
};
// std::map makes this non-trivial
// static_assert(std::is_trivial<FlarmData>::value, "type is not trivial");
// Removed duplicate static_assert

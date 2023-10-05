// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WaypointReaderBase.hpp"

#include <string_view>

class WaypointReaderOzi final : public WaypointReaderBase {
  /* ignore the first 4 lines */
  unsigned ignore_lines = 4;

public:
  explicit WaypointReaderOzi(WaypointFactory _factory)
    :WaypointReaderBase(_factory) {}

  [[gnu::pure]]
  static bool VerifyFormat(std::string_view contents) noexcept;

protected:
  /* virtual methods from class WaypointReaderBase */
  bool ParseLine(const TCHAR *line, Waypoints &way_points) override;
};

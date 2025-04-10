// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Engine/GlideSolvers/PolarCoefficients.hpp"
#include "Units/System.hpp"
#include "Math/Util.hpp"

#include <cassert>
#include <cstdint>

/**
 * Code specific to LX Navigation varios (e.g. LX1600).
 */
namespace LX1600 {
  enum class SCMode : uint8_t {
    EXTERNAL = 0,
    ON_CIRCLING = 1,
    AUTO_IAS = 2,
  };

  enum class SCControlMode : uint8_t {
    NORMAL = 0,
    INVERTED = 1,
    TASTER = 2,
  };

  /**
   * Store the current settings into the EEPROM of the device.
   */
  static inline void
  SaveToEEPROM(Port &port, OperationEnvironment &env)
  {
    PortWriteNMEA(port, "PFLX0,EEPROM", env);
  }

  /**
   * Initialize all settings to default, writes to EEPROM and resets unit.
   */
  static inline void
  FactoryReset(Port &port, OperationEnvironment &env)
  {
    PortWriteNMEA(port, "PFLX0,INITEEPROM", env);
  }

  static inline void
  SetupNMEA(Port &port, OperationEnvironment &env)
  {
    /*
     * This line sets the requested NMEA sentences on the device.
     * LXWP0: every second
     * LXWP1+3+5: once every 60 seconds
     * LXWP2: once every 10 seconds
     */
    PortWriteNMEA(port, "PFLX0,LXWP0,1,LXWP1,60,LXWP2,10,LXWP3,60,LXWP5,60", env);
  }

  /**
   * Set the MC setting of the LX16xx vario
   * @param mc in m/s
   */
  static inline void
  SetMacCready(Port &port, OperationEnvironment &env, double mc)
  {
    assert(mc >= 0.0 && mc <= 5.0);

    char buffer[32];
    sprintf(buffer, "PFLX2,%1.1f,,,,,,", mc);
    PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the ballast setting of the LX16xx vario
   * @param overload 1.0 - 1.5 (100 - 140%)
   */
  static inline void
  SetBallast(Port &port, OperationEnvironment &env, double overload)
  {
    assert(overload >= 0.8 && overload <= 5);

    // This is a copy of the routine done in LK8000 for LX MiniMap, realized
    // by Lx developers.

    char buffer[100];
    sprintf(buffer, "PFLX2,,%.2f,,,,", overload);
    PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the bugs setting of the LX16xx vario
   * @param bugs 0 - 30 %
   */
  static inline void
  SetBugs(Port &port, OperationEnvironment &env, unsigned bugs)
  {
    assert(bugs <= 30);

    // This is a copy of the routine done in LK8000 for LX MiniMap, realized
    // by Lx developers.

    char buffer[100];
    sprintf(buffer, "PFLX2,,,%u,,,", bugs);
    PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the altitude offset of the LX16xx vario
   * @param altitude_offset offset necessary to set QNE in ft (default=0)
   */
  static inline void
  SetAltitudeOffset(Port &port, OperationEnvironment &env,
                    double altitude_offset)
  {
    char buffer[100];
    sprintf(buffer, "PFLX3,%.2f,,,,,,,,,,,,", altitude_offset);
    PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the QNH setting of the LX16xx vario
   */
  static inline void
  SetQNH(Port &port, OperationEnvironment &env, const AtmosphericPressure &qnh)
  {
    assert(qnh.IsPlausible());

    auto altitude_offset = Units::ToUserUnit(
        -AtmosphericPressure::StaticPressureToPressureAltitude(qnh),
        Unit::FEET);

    SetAltitudeOffset(port, env, altitude_offset);
  }

  /**
   * Set the polar coefficients of the LX16xx vario
   *
   * These are the polar coefficients in LX format
   * (i.e. for v=(km/h*100) and w=(m/s))
   */
  static inline void
  SetPolar(Port &port, OperationEnvironment &env, double a, double b, double c)
  {
    char buffer[100];
    sprintf(buffer, "PFLX2,,,,%.2f,%.2f,%.2f,", a, b, c);
    PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the polar coefficients of the LX16xx vario
   * @param polar Polar coefficients in XCSoar format (SI, m/s)
   */
  static inline void
  SetPolar(Port &port, OperationEnvironment &env, const PolarCoefficients &polar)
  {
    // Convert from m/s to (km/h)/100
    auto polar_a = polar.a * 10000 / Square(3.6);
    auto polar_b = polar.b * 100 / 3.6;
    auto polar_c = polar.c;

    SetPolar(port, env, polar_a, polar_b, polar_c);
  }

  /**
   * Set the audio volume setting of the LX16xx vario
   * @param volume 0 - 100 %
   */
  static inline void
  SetVolume(Port &port, OperationEnvironment &env, unsigned volume)
  {
    assert(volume <= 100);

    if (volume > 99)
      volume = 99;

    char buffer[100];
    sprintf(buffer, "PFLX2,,,,,,,%u", volume);
    PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the filter settings of the LX16xx vario
   *
   * @param vario_filter filtering of vario in seconds (float) default=1
   * @param te_filter filtering of TE compensation in seconds (float)
   * (from 0.1 to 2.0 default=1.5)
   * @param te_level level of TE compensation in %
   * (from 50 to 150 default=0) 0 -> TECOMP = OFF
   */
  static inline void
  SetFilters(Port &port, OperationEnvironment &env,
             double vario_filter, double te_filter, unsigned te_level)
  {
    assert(te_filter >= 0.1 && te_filter <= 2.0);
    assert((te_level >= 50 && te_level <= 150) || te_level == 0);

    char buffer[100];
    sprintf(buffer, "PFLX3,,,%.1f,%.1f,%u",
            vario_filter, te_filter, te_level);
    PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the speed command settings of the LX16xx vario
   *
   * @param mode methods for automatic SC switch index (default=ON_CIRCLING)
   * @param deadband area of silence in SC mode (float)
   * (from 0 to 10.0 m/s, 1.0 = silence between +1m/s and -1m/s, default=1)
   * @param control_mode external switch/taster function (default=INVERTED)
   * @param threshold_speed speed of automatic switch from vario to sc mode
   * (if SCMODE == 2) (from 50 to 150 km/h, default=110)
   */
  static inline void
  SetSCSettings(Port &port, OperationEnvironment &env,
                SCMode mode, double deadband, SCControlMode control_mode,
                double threshold_speed = 0)
  {
    assert((unsigned)mode <= (unsigned)SCMode::AUTO_IAS);
    assert(deadband >= 0 && deadband <= 10);
    assert((unsigned)control_mode <= (unsigned)SCControlMode::TASTER);
    assert(mode != SCMode::AUTO_IAS ||
           (threshold_speed >= 50 && threshold_speed <= 150));

    char buffer[100];
    if (mode == SCMode::AUTO_IAS)
      sprintf(buffer, "PFLX3,,%u,,,,,,%.1f,%u,%.0f",
              (unsigned)mode, deadband, (unsigned)control_mode,
              threshold_speed);
    else
      sprintf(buffer, "PFLX3,,%u,,,,,,%.1f,%u",
              (unsigned)mode, deadband, (unsigned)control_mode);

    PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the vario settings of the LX16xx vario
   *
   * @param avg_time averaging time in seconds for integrator
   * (between 5s and 30s, default=25)
   * @param range range of the vario display (2.5, 5.0 or 10.0, default=5.0)
   */
  static inline void
  SetVarioSettings(Port &port, OperationEnvironment &env,
                   unsigned avg_time, double range)
  {
    assert(avg_time >= 5 && avg_time <= 30);
    assert(range >= 2.5 && range <= 10);

    char buffer[100];
    sprintf(buffer, "PFLX3,,,,,,%u,%.1f", avg_time, range);

    PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the Smart VARIO filtering
   * @param filter filter setting in m/s^2
   */
  static inline void
  SetSmartDiffFilter(Port &port, OperationEnvironment &env, double filter)
  {
    char buffer[100];
    sprintf(buffer, "PFLX3,,,,,,,,,,,%.1f", filter);

    PortWriteNMEA(port, buffer, env);
  }

  /**
   * Set the time offset of the LX16xx vario
   * @param offset time offset in hours
   */
  static inline void
  SetTimeOffset(Port &port, OperationEnvironment &env, int offset)
  {
    assert(offset >= -14 && offset <= 14);

    char buffer[100];
    sprintf(buffer, "PFLX3,,,,,,,,,,,,,%d", offset);

    PortWriteNMEA(port, buffer, env);
  }
}

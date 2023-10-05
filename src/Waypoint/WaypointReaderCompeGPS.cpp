// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointReaderCompeGPS.hpp"
#include "Waypoint/Waypoints.hpp"
#include "io/LineReader.hpp"
#include "Geo/UTM.hpp"
#include "util/StringSplit.hxx"

static bool
ParseAngle(const TCHAR *&src, Angle &angle) noexcept
{
  // 41.234234N

  TCHAR *endptr;

  // Parse numerical value
  double value = _tcstod(src, &endptr);
  if (endptr == src)
    return false;

  src = endptr;
  angle = Angle::Degrees(value);

  // Skip until next whitespace and look for NSEW signs
  bool found = false;
  while (*src != _T(' ') && *src != _T('\0')) {
    if (!found) {
      if (*src == _T('N') || *src == _T('n') ||
          *src == _T('E') || *src == _T('e')) {
        found = true;
      } else if (*src == _T('S') || *src == _T('s') ||
                 *src == _T('W') || *src == _T('w')) {
        found = true;
        angle.Flip();
      }
    }

    src++;
  }

  return found;
}

static bool
ParseLocation(const TCHAR *&src, GeoPoint &p) noexcept
{
  // A 41.234234N 7.234424W

  // Ignore but require 'A' placeholder
  if (*src != _T('A'))
    return false;

  src++;

  // Skip whitespace
  while (*src == _T(' '))
    src++;

  Angle lat, lon;
  if (!ParseAngle(src, lat) || !ParseAngle(src, lon))
    return false;

  p.longitude = lon;
  p.latitude = lat;

  // ensure longitude is within -180:180
  p.Normalize();

  return true;
}

static bool
ParseLocationUTM(const TCHAR *&src, GeoPoint &p) noexcept
{
  // 31T 318570 4657569

  TCHAR *endptr;

  // Parse zone number
  long zone_number = _tcstol(src, &endptr, 10);
  if (endptr == src)
    return false;

  src = endptr;
  char zone_letter = src[0];

  src++;
  long easting = _tcstol(src, &endptr, 10);
  if (endptr == src || *endptr != _T(' '))
    return false;

  src = endptr;
  long northing = _tcstol(src, &endptr, 10);
  if (endptr == src || *endptr != _T(' '))
    return false;

  UTM u(zone_number, zone_letter, easting, northing);
  p = u.ToGeoPoint();

  // ensure longitude is within -180:180
  p.Normalize();

  src = endptr;

  return true;
}

static bool
ParseAltitude(const TCHAR *&src, double &dest) noexcept
{
  TCHAR *endptr;
  double value = _tcstod(src, &endptr);
  if (endptr == src)
    return false;

  dest = value;
  src = endptr;
  return true;
}

bool
WaypointReaderCompeGPS::ParseLine(const TCHAR *line, Waypoints &waypoints)
{
  /*
   * G  WGS 84
   * U  1
   * W  IT05FC A 46.9121939503ºN 11.9605922700°E 27-MAR-62 00:00:00 566.000000 Ahornach Sand, Ahornach LP, GS und HG
   * w  Waypoint,0,-1.0,16777215,255,0,0,7,,0.0,
   * W  IT05FB A 46.9260440931ºN 11.9676733017°E 27-MAR-62 00:00:00 1425.000000 Ahornach Sand, Ahornach SP, GS und HG
   * w  Waypoint,0,-1.0,16777215,255,0,0,7,,0.0,
   *
   * W ShortName 31T 318570 4657569 27-MAR-62 00:00:00 0 some Comments
   * W ShortName A 41.234234N 7.234424W 27-MAR-62 00:00:00 0 Comments
   */

  // Skip projection and file encoding information
  if (*line == _T('G') || *line == _T('B'))
    return true;

  // Check for format: UTM or LatLon
  if (StringStartsWith(line, _T("U  0"))) {
    is_utm = true;
    return true;
  }

  // Skip non-waypoint lines
  if (*line != _T('W'))
    return true;

  // Skip W indicator and whitespace
  line++;
  while (*line == _T(' '))
    line++;

  // Find next space delimiter, skip shortname
  const TCHAR *name = line;
  const TCHAR *space = _tcsstr(line, _T(" "));
  if (space == nullptr)
    return false;

  unsigned name_length = space - line;
  if (name_length == 0)
    return false;

  line = space;
  while (*line == _T(' '))
    line++;

  // Parse location
  GeoPoint location;
  if (!(is_utm
        ? ParseLocationUTM(line, location)
        : ParseLocation(line, location)))
    return false;

  // Skip whitespace
  while (*line == _T(' '))
    line++;

  // Skip unused date field
  line = _tcsstr(line, _T(" "));
  if (line == nullptr)
    return false;

  line++;

  // Skip unused time field
  line = _tcsstr(line, _T(" "));
  if (line == nullptr)
    return false;

  line++;

  // Create new waypoint instance
  Waypoint waypoint = factory.Create(location);
  waypoint.name.assign(name, name_length);

  // Parse altitude
  if (ParseAltitude(line, waypoint.elevation))
    waypoint.has_elevation = true;
  else
    factory.FallbackElevation(waypoint);

  // Skip whitespace
  while (*line == _T(' '))
    line++;

  // Parse waypoint name
  waypoint.comment.assign(line);

  waypoints.Append(std::move(waypoint));
  return true;
}

bool
WaypointReaderCompeGPS::VerifyFormat(std::string_view contents) noexcept
{
  // Ignore optional line with encoding information
  if (contents.starts_with("B "))
    contents = Split(contents, '\n').second;

  return contents.starts_with("G  WGS 84");
}

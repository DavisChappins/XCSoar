/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Profile/AirspaceConfig.hpp"
#include "Profile/Profile.hpp"
#include "Screen/Features.hpp"
#include "Look/AirspaceLook.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Util/Macros.hpp"

static const TCHAR *
MakeAirspaceSettingName(TCHAR *buffer, const TCHAR *prefix, unsigned n)
{
  _tcscpy(buffer, prefix);
  _stprintf(buffer + _tcslen(buffer), _T("%u"), n);

  return buffer;
}

/**
 * This function and the "ColourXX" profile keys are deprecated and
 * are only used as a fallback for old profiles.
 *
 * @see Load(unsigned, AirspaceClassRendererSettings &)
 */
static bool
GetAirspaceColor(unsigned i, Color &color)
{
  TCHAR name[64];
  MakeAirspaceSettingName(name, _T("Colour"), i);

  // Try to load the hex color directly
  if (Profile::GetColor(name, color))
    return true;

  // Try to load an indexed preset color (legacy, < 6.3)
  unsigned index;
  if (!Profile::Get(name, index))
    return false;

  // Adjust index if the user has configured a preset color out of range
  if (index >= ARRAY_SIZE(AirspaceLook::preset_colors))
    index = 0;

  // Assign configured preset color
  color = AirspaceLook::preset_colors[index];
  return true;
}

void
Profile::Load(AirspaceRendererSettings &settings)
{
  Get(szProfileAirspaceBlackOutline, settings.black_outline);
  GetEnum(szProfileAltMode, settings.altitude_mode);
  Get(szProfileClipAlt, settings.clip_altitude);

#ifndef ENABLE_OPENGL
  Get(szProfileAirspaceTransparency, settings.transparency);
#endif

  GetEnum(szProfileAirspaceFillMode, settings.fill_mode);

  for (unsigned i = 0; i < AIRSPACECLASSCOUNT; i++)
    Load(i, settings.classes[i]);
}

void
Profile::Load(unsigned i, AirspaceClassRendererSettings &settings)
{
  TCHAR name[64];
  MakeAirspaceSettingName(name, _T("AirspaceMode"), i);

  unsigned value;
  if (Get(name, value))
    settings.display = (value & 0x1) != 0;

#ifdef HAVE_HATCHED_BRUSH
  MakeAirspaceSettingName(name, _T("Brush"), i);
  Get(name, settings.brush);
  if (settings.brush >= ARRAY_SIZE(AirspaceLook::brushes))
    settings.brush = 0;
#endif

  MakeAirspaceSettingName(name, _T("AirspaceBorderColor"), i);
  if (!GetColor(name, settings.border_color))
    GetAirspaceColor(i, settings.border_color);

  MakeAirspaceSettingName(name, _T("AirspaceFillColor"), i);
  if (!GetColor(name, settings.fill_color))
    GetAirspaceColor(i, settings.fill_color);

  MakeAirspaceSettingName(name, _T("AirspaceBorderWidth"), i);
  Get(name, settings.border_width);
}

void
Profile::Load(AirspaceComputerSettings &settings)
{
  Get(szProfileAirspaceWarning, settings.enable_warnings);
  Get(szProfileAltMargin, settings.warnings.altitude_warning_margin);
  Get(szProfileWarningTime, settings.warnings.warning_time);
  Get(szProfileAcknowledgementTime, settings.warnings.acknowledgement_time);

  TCHAR name[64];
  unsigned value;
  for (unsigned i = 0; i < AIRSPACECLASSCOUNT; i++) {
    MakeAirspaceSettingName(name, _T("AirspaceMode"), i);
    if (Get(name, value))
      settings.warnings.class_warnings[i] = (value & 0x2) != 0;
  }
}

void
Profile::SetAirspaceMode(unsigned i, bool display, bool warning)
{
  TCHAR name[64];
  MakeAirspaceSettingName(name, _T("AirspaceMode"), i);

  int value = 0;
  if (display)
    value |= 0x1;
  if (warning)
    value |= 0x2;

  Set(name, value);
}

void
Profile::SetAirspaceBorderWidth(unsigned i, unsigned border_width)
{
  TCHAR name[64];
  MakeAirspaceSettingName(name, _T("AirspaceBorderWidth"), i);
  Set(name, border_width);
}

void
Profile::SetAirspaceBorderColor(unsigned i, const Color &color)
{
  TCHAR name[64];
  MakeAirspaceSettingName(name, _T("AirspaceBorderColor"), i);
  SetColor(name, color);
}

void
Profile::SetAirspaceFillColor(unsigned i, const Color &color)
{
  TCHAR name[64];
  MakeAirspaceSettingName(name, _T("AirspaceFillColor"), i);
  SetColor(name, color);
}

void
Profile::SetAirspaceBrush(unsigned i, int brush_index)
{
  TCHAR name[64];
  MakeAirspaceSettingName(name, _T("Brush"), i);
  Set(name, brush_index);
}

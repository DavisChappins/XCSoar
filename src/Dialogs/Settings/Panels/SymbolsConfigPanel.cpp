// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SymbolsConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Dialogs/TimeEntry.hpp"
#include "Form/DataField/String.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "MapSettings.hpp"

enum ControlIndex {
  DISPLAY_TRACK_BEARING,
  ENABLE_FLARM_MAP,
  FADE_TRAFFIC,
  TRAIL_LENGTH,
  TRAIL_DRIFT,
  TRAIL_TYPE,
  TRAIL_WIDTH,
  ENABLE_DETOUR_COST_MARKERS,
  AIRCRAFT_SYMBOL,
  WIND_ARROW_STYLE,
  SKYLINES_TRAFFIC_MAP_MODE,
  ARRIVAL_TIME_RING,
};

class SymbolsConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  SymbolsConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void ShowTrailControls(bool show);

  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
SymbolsConfigPanel::ShowTrailControls(bool show)
{
  SetRowVisible(TRAIL_DRIFT, show);
  SetRowVisible(TRAIL_TYPE, show);
  SetRowVisible(TRAIL_WIDTH, show);
}

void
SymbolsConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(TRAIL_LENGTH, df)) {
    const DataFieldEnum &dfe = (const DataFieldEnum &)df;
    TrailSettings::Length trail_length = (TrailSettings::Length)dfe.GetValue();
    ShowTrailControls(trail_length != TrailSettings::Length::OFF);
  }
}

static bool
ArrivalTimeRingEditCallback([[maybe_unused]] const TCHAR *caption, DataField &df,
                           [[maybe_unused]] const TCHAR *help_text) noexcept
{
  unsigned minutes_of_day = 17 * 60 + 0; // Default to 5:00 PM (1700)
  Profile::Get(ProfileKeys::ArrivalTimeRingTime, minutes_of_day);
  RoughTime time = RoughTime::FromMinuteOfDay(minutes_of_day);
  if (TimeEntryDialog(_("Arrival time ring"), time, RoughTimeDelta::FromMinutes(0))) {
    unsigned new_minutes_of_day = time.GetMinuteOfDay();
    Profile::Set(ProfileKeys::ArrivalTimeRingTime, new_minutes_of_day);
    
    // Update the displayed text
    TCHAR time_text[16];
    _stprintf(time_text, _T("%02u:%02u"), new_minutes_of_day / 60, new_minutes_of_day % 60);
    
    // Update the DataField
    DataFieldString &dfs = (DataFieldString &)df;
    dfs.ModifyValue(time_text);
    
    return true;
  }
  return false;
}

static constexpr StaticEnumChoice ground_track_mode_list[] = {
  { DisplayGroundTrack::OFF, N_("Off"), N_("Disable display of ground track line.") },
  { DisplayGroundTrack::ON, N_("On"), N_("Always display ground track line.") },
  { DisplayGroundTrack::AUTO, N_("Auto"), N_("Display ground track line if there is a significant difference to plane heading.") },
  nullptr
};

static constexpr StaticEnumChoice trail_length_list[] = {
  { TrailSettings::Length::OFF, N_("Off") },
  { TrailSettings::Length::LONG, N_("Long") },
  { TrailSettings::Length::SHORT, N_("Short") },
  { TrailSettings::Length::FULL, N_("Full") },
  nullptr
};

static constexpr StaticEnumChoice trail_type_list[] = {
  { TrailSettings::Type::VARIO_1, N_("Vario #1"), N_("Within lift areas "
    "lines get displayed green and thicker, while sinking lines are shown brown and thin. "
    "Zero lift is presented as a grey line.") },
  { TrailSettings::Type::VARIO_1_DOTS, N_("Vario #1 (with dots)"), N_("The same "
    "colour scheme as the previous, but with dotted lines while sinking.") },
  { TrailSettings::Type::VARIO_2, N_("Vario #2"), N_("The climb colour "
    "for this scheme is orange to red, sinking is displayed as light blue to dark blue. "
    "Zero lift is presented as a yellow line.") },
  { TrailSettings::Type::VARIO_2_DOTS, N_("Vario #2 (with dots)"), N_("The same "
    "colour scheme as the previous, but with dotted lines while sinking.") },
  { TrailSettings::Type::VARIO_DOTS_AND_LINES,
    N_("Vario-scaled dots and lines"),
    N_("Vario-scaled dots with lines. "
       "Orange to red = climb. Light blue to dark blue = sink. "
       "Zero lift is presented as a yellow line.") },
  { TrailSettings::Type::VARIO_EINK, N_("Vario E-ink"), N_("E-ink friendly color scheme, lighter and thicker dots means lift while darker and thinner means sink.") },
  { TrailSettings::Type::ALTITUDE, N_("Altitude"), N_("The colour scheme corresponds to the height.") },
  nullptr
};

static constexpr StaticEnumChoice  aircraft_symbol_list[] = {
  { AircraftSymbol::SIMPLE, N_("Simple"),
    N_("Simplified line graphics, black with white contours.") },
  { AircraftSymbol::SIMPLE_LARGE, N_("Simple (large)"),
    N_("Enlarged simple graphics.") },
  { AircraftSymbol::DETAILED, N_("Detailed"),
    N_("Detailed rendered aircraft graphics.") },
  { AircraftSymbol::HANGGLIDER, N_("HangGlider"),
    N_("Simplified hang glider as line graphics, white with black contours.") },
  { AircraftSymbol::PARAGLIDER, N_("ParaGlider"),
    N_("Simplified para glider as line graphics, white with black contours.") },
  nullptr
};

static constexpr StaticEnumChoice wind_arrow_list[] = {
  { WindArrowStyle::NO_ARROW, N_("Off"), N_("No wind arrow is drawn.") },
  { WindArrowStyle::ARROW_HEAD, N_("Arrow head"), N_("Draws an arrow head only.") },
  { WindArrowStyle::FULL_ARROW, N_("Full arrow"), N_("Draws an arrow head with a dashed arrow line.") },
  nullptr
};

static constexpr StaticEnumChoice skylines_map_mode_list[] = {
  { DisplaySkyLinesTrafficMapMode::OFF, N_("Off"), N_("No SkyLines traffic is drawn.") },
  { DisplaySkyLinesTrafficMapMode::SYMBOL, N_("Symbol"), N_("Draws the SkyLines symbol only.") },
  { DisplaySkyLinesTrafficMapMode::SYMBOL_NAME, N_("Symbol and Name"), N_("Draws the SkyLines symbol with name.") },
  nullptr
};

void
SymbolsConfigPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                            [[maybe_unused]] const PixelRect &rc) noexcept
{
  const MapSettings &settings_map = CommonInterface::GetMapSettings();

  AddEnum(_("Ground track"),
          _("Display the ground track as a grey line on the map."),
          ground_track_mode_list, (unsigned)settings_map.display_ground_track);

  AddBoolean(_("FLARM traffic"), _("This enables the display of FLARM traffic on the map window."),
             settings_map.show_flarm_on_map);

  AddBoolean(_("Fade traffic"), _("Keep showing traffic for a while after it has disappeared."),
             settings_map.fade_traffic);

  AddEnum(_("Trail length"),
          _("Determines whether and how long a snail trail is drawn behind the glider."),
          trail_length_list,
          (unsigned)settings_map.trail.length, this);
  SetExpertRow(TRAIL_LENGTH);

  AddBoolean(_("Trail drift"),
             _("Determines whether the snail trail is drifted with the wind when displayed in "
               "circling mode. Switched Off, "
               "the snail trail stays uncompensated for wind drift."),
             settings_map.trail.wind_drift_enabled);
  SetExpertRow(TRAIL_DRIFT);

  AddEnum(_("Trail type"),
          _("Sets the type of the snail trail display."), trail_type_list, (int)settings_map.trail.type);
  SetExpertRow(TRAIL_TYPE);

  AddBoolean(_("Trail scaled"),
             _("If set to ON the snail trail width is scaled according to the vario signal."),
             settings_map.trail.scaling_enabled);
  SetExpertRow(TRAIL_WIDTH);

  AddBoolean(_("Detour cost markers"),
             _("If the aircraft heading deviates from the current waypoint, markers are displayed "
                 "at points ahead of the aircraft. The value of each marker is the extra distance "
                 "required to reach that point as a percentage of straight-line distance to the waypoint."),
             settings_map.detour_cost_markers_enabled);
  SetExpertRow(ENABLE_DETOUR_COST_MARKERS);

  AddEnum(_("Aircraft symbol"), nullptr, aircraft_symbol_list,
          (unsigned)settings_map.aircraft_symbol);
  SetExpertRow(AIRCRAFT_SYMBOL);

  AddEnum(_("Wind arrow"), _("Determines the way the wind arrow is drawn on the map."),
          wind_arrow_list, (unsigned)settings_map.wind_arrow_style);
  SetExpertRow(WIND_ARROW_STYLE);

  AddEnum(_("SkyLines traffic mode"),
           _("Show the SkyLines traffic symbols/names on the map, downloaded from the SkyLines server."),
           skylines_map_mode_list, (unsigned)settings_map.skylines_traffic_map_mode);

  // Get the current arrival time ring time
  unsigned minutes_of_day = 17 * 60 + 0; // Default to 5:00 PM (1700)
  Profile::Get(ProfileKeys::ArrivalTimeRingTime, minutes_of_day);
  
  // Format the time as a string
  TCHAR time_text[16];
  _stprintf(time_text, _T("%02u:%02u"), minutes_of_day / 60, minutes_of_day % 60);
  
  // Add a text field that shows the current time
  WndProperty *wp = AddText(_("Arrival time ring"),
                          _("Arrival time ring draws a ring at your final waypoint (goto or task) that determines if you will arrive before or after the specified time. If you are inside the ring you will arrive before the time, if you are outside of the ring you will arrive after the time. Ring radius is set by the time difference between now and then and your climb/cruise MacCready speed."),
                          time_text);
  
  // Set a callback for when the field is clicked
  wp->SetEditCallback(ArrivalTimeRingEditCallback);

  ShowTrailControls(settings_map.trail.length != TrailSettings::Length::OFF);
}

bool
SymbolsConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  MapSettings &settings_map = CommonInterface::SetMapSettings();

  changed |= SaveValueEnum(DISPLAY_TRACK_BEARING, ProfileKeys::DisplayTrackBearing,
                           settings_map.display_ground_track);

  changed |= SaveValue(ENABLE_FLARM_MAP, ProfileKeys::EnableFLARMMap,
                       settings_map.show_flarm_on_map);

  changed |= SaveValue(FADE_TRAFFIC, ProfileKeys::FadeTraffic,
                       settings_map.fade_traffic);

  changed |= SaveValueEnum(TRAIL_LENGTH, ProfileKeys::SnailTrail, settings_map.trail.length);

  changed |= SaveValue(TRAIL_DRIFT, ProfileKeys::TrailDrift, settings_map.trail.wind_drift_enabled);

  changed |= SaveValueEnum(TRAIL_TYPE, ProfileKeys::SnailType, settings_map.trail.type);

  changed |= SaveValue(TRAIL_WIDTH, ProfileKeys::SnailWidthScale,
                       settings_map.trail.scaling_enabled);

  changed |= SaveValue(ENABLE_DETOUR_COST_MARKERS, ProfileKeys::DetourCostMarker,
                       settings_map.detour_cost_markers_enabled);

  changed |= SaveValueEnum(AIRCRAFT_SYMBOL, ProfileKeys::AircraftSymbol, settings_map.aircraft_symbol);

  changed |= SaveValueEnum(WIND_ARROW_STYLE, ProfileKeys::WindArrowStyle, settings_map.wind_arrow_style);

  changed |= SaveValueEnum(SKYLINES_TRAFFIC_MAP_MODE, ProfileKeys::SkyLinesTrafficMapMode,
                           settings_map.skylines_traffic_map_mode);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateSymbolsConfigPanel()
{
  return std::make_unique<SymbolsConfigPanel>();
}

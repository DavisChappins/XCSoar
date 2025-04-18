// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SafetyFactorsConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Float.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Language/Language.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

enum ControlIndex {
  ArrivalHeight,
  TerrainHeight,
  AlternateMode,
  PolarDegradation,
  AutoBugs,
  SafetyMC,
  RiskFactor,
  TurnBackMarker,
};

class SafetyFactorsConfigPanel final : public RowFormWidget {
public:
  SafetyFactorsConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
SafetyFactorsConfigPanel::Prepare(ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();
  const TaskBehaviour &task_behaviour = settings_computer.task;

  AddFloat(_("Arrival height"),
           _("The height above terrain that the glider should arrive at for a safe landing."),
           _T("%.0f %s"), _T("%.0f"),
           0, 2000, 10, false,
           UnitGroup::ALTITUDE, task_behaviour.safety_height_arrival);

  AddFloat(_("Terrain height"),
           _("The height above terrain that the glider must clear during final glide."),
           _T("%.0f %s"), _T("%.0f"),
           0, 1000, 10, false,
           UnitGroup::ALTITUDE, task_behaviour.route_planner.safety_height_terrain);

  static constexpr StaticEnumChoice abort_task_mode_list[] = {
    { AbortTaskMode::SIMPLE, N_("Simple"),
      N_("The alternates will only be sorted by waypoint type (airport/outlanding field) and arrival height.") },
    { AbortTaskMode::TASK, N_("Task"),
      N_("The sorting will also take the current task direction into account.") },
    { AbortTaskMode::HOME, N_("Home"),
      N_("The sorting will try to find landing options in the current direction to the configured home waypoint.") },
    nullptr
  };

  AddEnum(_("Alternates mode"),
          _("Determines sorting of alternates in the alternates dialog and in abort mode."),
          abort_task_mode_list, (unsigned)task_behaviour.abort_task_mode);

  AddFloat(_("Polar degradation"), /* xgettext:no-c-format */
           _("A permanent polar degradation. "
             "0% means no degradation, "
             "50% indicates the glider's sink rate is doubled."),
           _T("%.0f %%"), _T("%.0f"),
           0, 50, 1, false,
           (1 - settings_computer.polar.degradation_factor) * 100);
  SetExpertRow(PolarDegradation);

  AddBoolean(_("Auto bugs"), /* xgettext:no-c-format */
           _("If enabled, adds 1% to the bugs setting after each full hour while flying."),
             settings_computer.polar.auto_bugs);
  SetExpertRow(AutoBugs);

  AddFloat(_("Safety MC"),
           _("The MacCready setting used, when safety MC is enabled for reach calculations, in task abort mode and for determining arrival altitude at airfields."),
           _T("%.1f %s"), _T("%.1f"),
           0, Units::ToUserVSpeed(10), GetUserVerticalSpeedStep(),
           false, UnitGroup::VERTICAL_SPEED, task_behaviour.safety_mc);
  SetExpertRow(SafetyMC);
  DataFieldFloat &safety_mc = (DataFieldFloat &)GetDataField(SafetyMC);
  safety_mc.SetFormat(GetUserVerticalSpeedFormat(false, false));

  AddFloat(_("STF risk factor"),
           _("The STF risk factor reduces the MacCready setting used to calculate speed to fly as the glider gets low, in order to compensate for risk. Set to 0.0 for no compensation, 1.0 scales MC linearly with current height (with reference to height of the maximum climb). If considered, 0.3 is recommended."),
           _T("%.1f %s"), _T("%.1f"),
           0, 1, 0.1, false,
           task_behaviour.risk_gamma);
 SetExpertRow(RiskFactor);

 AddBoolean(_("Turn back marker"),
            _("Show a marker indicating the point of no return based on current conditions."),
            task_behaviour.turn_back_marker_enabled);
}

bool
SafetyFactorsConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  TaskBehaviour &task_behaviour = settings_computer.task;

  changed |= SaveValue(ArrivalHeight, UnitGroup::ALTITUDE,
                       ProfileKeys::SafetyAltitudeArrival,
                       task_behaviour.safety_height_arrival);

  changed |= SaveValue(TerrainHeight, UnitGroup::ALTITUDE,
                       ProfileKeys::SafetyAltitudeTerrain,
                       task_behaviour.route_planner.safety_height_terrain);

  changed |= SaveValueEnum(AlternateMode, ProfileKeys::AbortTaskMode,
                           task_behaviour.abort_task_mode);

  double degradation = (1 - settings_computer.polar.degradation_factor) * 100;
  if (SaveValue(PolarDegradation, degradation)) {
    settings_computer.polar.SetDegradationFactor(1 - degradation / 100);
    Profile::Set(ProfileKeys::PolarDegradation,
                 settings_computer.polar.degradation_factor);
    backend_components->SetTaskPolar(settings_computer.polar);
    changed = true;
  }

  if (SaveValue(AutoBugs, settings_computer.polar.auto_bugs)) {
    Profile::Set(ProfileKeys::AutoBugs, settings_computer.polar.auto_bugs);
    changed = true;
  }

  if (SaveValue(SafetyMC, UnitGroup::VERTICAL_SPEED, task_behaviour.safety_mc)) {
    Profile::Set(ProfileKeys::SafetyMacCready,
                 iround(task_behaviour.safety_mc * 10));
    changed = true;
  }

  if (SaveValue(RiskFactor, task_behaviour.risk_gamma)) {
    Profile::Set(ProfileKeys::RiskGamma,
                 iround(task_behaviour.risk_gamma * 10));
    changed = true;
  }

  if (SaveValue(TurnBackMarker, task_behaviour.turn_back_marker_enabled)) {
    Profile::Set(ProfileKeys::TurnBackMarkerEnabled, task_behaviour.turn_back_marker_enabled);
    changed = true;
  }

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateSafetyFactorsConfigPanel()
{
  return std::make_unique<SafetyFactorsConfigPanel>();
}

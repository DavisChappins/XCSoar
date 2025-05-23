// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Interface.hpp"
#include "Task/TypeStrings.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Widget/RowFormWidget.hpp"
#include "TaskDefaultsConfigPanel.hpp"
#include "UIGlobals.hpp"

using namespace std::chrono;

enum ControlIndex {
  StartType,
  StartRadius,
  spacer_1,
  FinishType,
  FinishRadius,
  spacer_2,
  TurnpointType,
  TurnpointRadius,
  spacer_3,
  TaskType,
  AATMinTime,
  AATTimeMargin,
  ArrivalRingAAT
};

class TaskDefaultsConfigPanel final
  : public RowFormWidget, DataFieldListener {
public:
  TaskDefaultsConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void SetStartLabel();
  void SetFinishLabel();

  /* methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
TaskDefaultsConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(StartType, df))
    SetStartLabel();
  else if (IsDataField(FinishType, df))
    SetFinishLabel();
}

static const TCHAR *const Caption_GateWidth = N_("Gate width");
static const TCHAR *const Caption_Radius = N_("Radius");

void
TaskDefaultsConfigPanel::SetStartLabel()
{
  WndProperty &wp = GetControl(StartRadius);

  if ((TaskPointFactoryType)GetValueEnum(StartType) == TaskPointFactoryType::START_LINE)
    wp.SetCaption(gettext(Caption_GateWidth));
  else
    wp.SetCaption(gettext(Caption_Radius));
}

void
TaskDefaultsConfigPanel::SetFinishLabel()
{
  WndProperty &wp = GetControl(FinishRadius);

  if ((TaskPointFactoryType)GetValueEnum(FinishType) == TaskPointFactoryType::FINISH_LINE)
    wp.SetCaption(gettext(Caption_GateWidth));
  else
    wp.SetCaption(gettext(Caption_Radius));
}

static void
FillPointTypes(DataFieldEnum &df,
               const LegalPointSet &l,
               TaskPointFactoryType value)
{
  df.EnableItemHelp(true);

  for (unsigned i = 0; i < l.N; ++i) {
    const TaskPointFactoryType type = TaskPointFactoryType(i);
    if (!l.Contains(type))
      continue;

    df.addEnumText(OrderedTaskPointName(type), (unsigned)type,
                   OrderedTaskPointDescription(type));
  }

  df.SetValue(value);
}

static void
FillPointTypes(WndProperty &wp,
               const LegalPointSet &l,
               TaskPointFactoryType value)
{
  FillPointTypes(*(DataFieldEnum *)wp.GetDataField(), l, value);
  wp.RefreshDisplay();
}

void
TaskDefaultsConfigPanel::Prepare(ContainerWindow &parent,
                                 const PixelRect &rc) noexcept
{
  WndProperty *wp;
  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();
  const TaskBehaviour &task_behaviour = settings_computer.task;
  OrderedTask temptask(task_behaviour);
  temptask.SetFactory(TaskFactoryType::RACING);

  RowFormWidget::Prepare(parent, rc);

  wp = AddEnum(_("Start point"),
               _("Default start type for new tasks you create."),
               this);
  FillPointTypes(*wp, temptask.GetFactory().GetValidStartTypes(),
                 task_behaviour.sector_defaults.start_type);

  AddFloat(Caption_GateWidth, _("Default radius or gate width of the start zone for new tasks."),
           _T("%.1f %s"), _T("%.1f"), 0.1, 100, 1.0, true, UnitGroup::DISTANCE,
           task_behaviour.sector_defaults.start_radius);

  AddSpacer();

  wp = AddEnum(_("Finish point"),
               _("Default finish type for new tasks you create."),
               this);
  FillPointTypes(*wp, temptask.GetFactory().GetValidFinishTypes(),
                 task_behaviour.sector_defaults.finish_type);

  AddFloat(Caption_GateWidth, _("Default radius or gate width of the finish zone in new tasks."),
           _T("%.1f %s"), _T("%.1f"), 0.1, 100, 1.0, true, UnitGroup::DISTANCE,
           task_behaviour.sector_defaults.finish_radius);

  AddSpacer();

  wp = AddEnum(_("Turn point"), _("Default turn point type for new tasks you create."));
  FillPointTypes(*wp, temptask.GetFactory().GetValidIntermediateTypes(),
                 task_behaviour.sector_defaults.turnpoint_type);

  AddFloat(Caption_Radius, _("Default radius of turnpoint cylinders and sectors in new tasks."),
           _T("%.1f %s"), _T("%.1f"), 0.1, 100, 1.0, true, UnitGroup::DISTANCE,
           task_behaviour.sector_defaults.turnpoint_radius);

  AddSpacer();

  wp = AddEnum(_("Task"), _("Default task type for new tasks you create."));
  if (wp) {
    const std::vector<TaskFactoryType> factory_types =
      temptask.GetFactoryTypes();
    DataFieldEnum* dfe = (DataFieldEnum*)wp->GetDataField();
    dfe->EnableItemHelp(true);

    for (unsigned i = 0; i < factory_types.size(); i++) {
      dfe->addEnumText(OrderedTaskFactoryName(factory_types[i]),
          (unsigned)factory_types[i], OrderedTaskFactoryDescription(
              factory_types[i]));
      if (factory_types[i] == task_behaviour.task_type_default)
        dfe->SetValue(factory_types[i]);
    }
    wp->RefreshDisplay();
  }

  AddDuration(_("AAT min. time"), _("Default AAT min. time for new AAT tasks."),
              minutes{1}, hours{10}, minutes{1},
              task_behaviour.ordered_defaults.aat_min_time);

  AddDuration(_("Optimisation margin"),
              _("Safety margin for AAT task optimisation.  Optimisation "
                "seeks to complete the task at the minimum time plus this margin time."),
              {}, minutes{30}, minutes{1},
              task_behaviour.optimise_targets_margin);
  SetExpertRow(AATTimeMargin);

  AddBoolean(_("Arrival ring AAT"),
             _("Show the arrival time ring based on AAT time remaining."),
             task_behaviour.arrival_ring_aat_enabled);

  SetStartLabel();
  SetFinishLabel();
}

bool
TaskDefaultsConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  ComputerSettings &settings_computer = CommonInterface::SetComputerSettings();
  TaskBehaviour &task_behaviour = settings_computer.task;

  changed |= SaveValueEnum(StartType, ProfileKeys::StartType, task_behaviour.sector_defaults.start_type);

  changed |= SaveValue(StartRadius, UnitGroup::DISTANCE, ProfileKeys::StartRadius,
                       task_behaviour.sector_defaults.start_radius);

  changed |= SaveValueEnum(TurnpointType, ProfileKeys::TurnpointType,
                           task_behaviour.sector_defaults.turnpoint_type);

  changed |= SaveValue(TurnpointRadius, UnitGroup::DISTANCE, ProfileKeys::TurnpointRadius,
                       task_behaviour.sector_defaults.turnpoint_radius);

  changed |= SaveValueEnum(FinishType, ProfileKeys::FinishType,
                           task_behaviour.sector_defaults.finish_type);

  changed |= SaveValue(FinishRadius, UnitGroup::DISTANCE, ProfileKeys::FinishRadius,
                       task_behaviour.sector_defaults.finish_radius);

  changed |= SaveValueEnum(TaskType, ProfileKeys::TaskType, task_behaviour.task_type_default);

  changed |= SaveValue(AATMinTime, ProfileKeys::AATMinTime,
                       task_behaviour.ordered_defaults.aat_min_time);

  changed |= SaveValue(AATTimeMargin, ProfileKeys::AATTimeMargin,
                        task_behaviour.optimise_targets_margin);

  changed |= SaveValue(ArrivalRingAAT, ProfileKeys::ArrivalRingAATEnabled,
                       task_behaviour.arrival_ring_aat_enabled);

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreateTaskDefaultsConfigPanel()
{
  return std::make_unique<TaskDefaultsConfigPanel>();
}

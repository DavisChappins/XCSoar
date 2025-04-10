// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/MapItemListDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Dialogs/Task/TaskDialogs.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Dialogs/Traffic/TrafficDialogs.hpp"
#include "Dialogs/Weather/WeatherDialog.hpp"
#include "Language/Language.hpp"
#include "MapSettings.hpp"
#include "MapWindow/Items/MapItem.hpp"
#include "MapWindow/Items/List.hpp"
#include "Renderer/MapItemListRenderer.hpp"
#include "Widget/ListWidget.hpp"
#include "Form/Button.hpp"
#include "Weather/Features.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "MainWindow.hpp"

#ifdef HAVE_NOAA
#include "Dialogs/Weather/NOAADetails.hpp"
#endif

static bool
HasDetails(const MapItem &item)
{
  switch (item.type) {
  case MapItem::Type::LOCATION:
  case MapItem::Type::ARRIVAL_ALTITUDE:
  case MapItem::Type::SELF:
  case MapItem::Type::THERMAL:
#ifdef HAVE_SKYLINES_TRACKING
  case MapItem::Type::SKYLINES_TRAFFIC:
#endif
    return false;

  case MapItem::Type::AIRSPACE:
  case MapItem::Type::WAYPOINT:
  case MapItem::Type::TASK_OZ:
  case MapItem::Type::TRAFFIC:
#ifdef HAVE_NOAA
  case MapItem::Type::WEATHER:
#endif
  case MapItem::Type::OVERLAY:
  case MapItem::Type::RASP:
    return true;

  case MapItem::Type::ARRIVAL_TIME_RING:
    // This item type has no details view.
    return false;
  }

  return false;
}

class MapItemListWidget final
  : public ListWidget {
  const MapItemList &list;

  const DialogLook &dialog_look;
  const MapSettings &settings;

  MapItemListRenderer renderer;

  Button *settings_button, *details_button, *cancel_button, *goto_button;
  Button *ack_button;

public:
  void CreateButtons(WidgetDialog &dialog);

public:
  MapItemListWidget(const MapItemList &_list,
                    const DialogLook &_dialog_look, const MapLook &_look,
                    const TrafficLook &_traffic_look,
                    const FinalGlideBarLook &_final_glide_look,
                    const MapSettings &_settings)
    :list(_list),
     dialog_look(_dialog_look),
     settings(_settings),
     renderer(_look, _traffic_look, _final_glide_look,
              _settings, CommonInterface::GetComputerSettings().utc_offset) {}

  unsigned GetCursorIndex() const {
    return GetList().GetCursorIndex();
  }

protected:
  void UpdateButtons() {
    const unsigned current = GetCursorIndex();
    details_button->SetEnabled(HasDetails(*list[current]));
    goto_button->SetEnabled(CanGotoItem(current));
    ack_button->SetEnabled(CanAckItem(current));
  }

  void OnGotoClicked();
  void OnAckClicked();

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  /* virtual methods from class List::Handler */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  void OnCursorMoved([[maybe_unused]] unsigned index) noexcept override {
    UpdateButtons();
  }

  bool CanActivateItem(unsigned index) const noexcept override {
    return HasDetails(*list[index]);
  }

  bool CanGotoItem(unsigned index) const noexcept {
    return CanGotoItem(*list[index]);
  }

  static bool CanGotoItem(const MapItem &item) noexcept {
    return backend_components->protected_task_manager &&
      item.type == MapItem::Type::WAYPOINT;
  }

  bool CanAckItem(unsigned index) const noexcept {
    return CanAckItem(*list[index]);
  }

  static bool CanAckItem(const MapItem &item) noexcept {
    const AirspaceMapItem &as_item = (const AirspaceMapItem &)item;

    return item.type == MapItem::Type::AIRSPACE &&
      backend_components->GetAirspaceWarnings() != nullptr &&
      !backend_components->GetAirspaceWarnings()->GetAckDay(*as_item.airspace);
  }

  void OnActivateItem(unsigned index) noexcept override;
};

void
MapItemListWidget::CreateButtons(WidgetDialog &dialog)
{
  details_button = dialog.AddButton(_("Details"), mrOK);

  goto_button = dialog.AddButton(_("Goto"), [this](){
    OnGotoClicked();
  });

  ack_button = dialog.AddButton(_("Ack Day"), [this](){
    OnAckClicked();
  });

  settings_button = dialog.AddButton(_("Settings"), [](){
    ShowMapItemListSettingsDialog();
  });

  cancel_button = dialog.AddButton(_("Close"), mrCancel);
}

void
MapItemListWidget::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  CreateList(parent, dialog_look, rc,
             renderer.CalculateLayout(dialog_look));

  GetList().SetLength(list.size());
  UpdateButtons();

  for (unsigned i = 0; i < list.size(); ++i) {
    const MapItem &item = *list[i];
    if (HasDetails(item) || CanGotoItem(item)) {
      GetList().SetCursorIndex(i);
      break;
    }
  }
}

void
MapItemListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                               unsigned idx) noexcept
{
  const MapItem &item = *list[idx];
  renderer.Draw(canvas, rc, item,
                &CommonInterface::Basic().flarm.traffic);

  if ((settings.item_list.add_arrival_altitude &&
       item.type == MapItem::Type::ARRIVAL_ALTITUDE) ||
      (!settings.item_list.add_arrival_altitude &&
       item.type == MapItem::Type::LOCATION)) {
    canvas.SelectBlackPen();
    canvas.DrawLine({rc.left, rc.bottom - 1}, {rc.right, rc.bottom - 1});
  }
}

void
MapItemListWidget::OnActivateItem([[maybe_unused]] unsigned index) noexcept
{
  details_button->Click();
}

inline void
MapItemListWidget::OnGotoClicked()
{
  if (!backend_components->protected_task_manager)
    return;

  unsigned index = GetCursorIndex();
  auto const &item = *list[index];

  assert(item.type == MapItem::Type::WAYPOINT);

  auto waypoint = ((const WaypointMapItem &)item).waypoint;
  backend_components->protected_task_manager->DoGoto(std::move(waypoint));
  cancel_button->Click();
  CommonInterface::main_window->FullRedraw();
}

inline void
MapItemListWidget::OnAckClicked()
{
  const AirspaceMapItem &as_item = *(const AirspaceMapItem *)
    list[GetCursorIndex()];
  backend_components->GetAirspaceWarnings()->AcknowledgeDay(as_item.airspace);
  UpdateButtons();
}

static int
ShowMapItemListDialog(const MapItemList &list,
                      const DialogLook &dialog_look, const MapLook &look,
                      const TrafficLook &traffic_look,
                      const FinalGlideBarLook &final_glide_look,
                      const MapSettings &settings)
{
  TWidgetDialog<MapItemListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           dialog_look, _("Map elements at this location"));
  dialog.SetWidget(list, dialog_look, look,
                   traffic_look, final_glide_look,
                   settings);
  dialog.GetWidget().CreateButtons(dialog);
  dialog.EnableCursorSelection();

  return dialog.ShowModal() == mrOK
    ? (int)dialog.GetWidget().GetCursorIndex()
    : -1;
}

static void
ShowMapItemDialog(const MapItem &item,
                  Waypoints *waypoints,
                  ProtectedAirspaceWarningManager *airspace_warnings)
{
  switch (item.type) {
  case MapItem::Type::LOCATION:
  case MapItem::Type::ARRIVAL_ALTITUDE:
  case MapItem::Type::SELF:
  case MapItem::Type::THERMAL:
#ifdef HAVE_SKYLINES_TRACKING
  case MapItem::Type::SKYLINES_TRAFFIC:
#endif
    break;

  case MapItem::Type::AIRSPACE:
    dlgAirspaceDetails(((const AirspaceMapItem &)item).airspace,
                       airspace_warnings);
    break;
  case MapItem::Type::WAYPOINT:
    dlgWaypointDetailsShowModal(waypoints,
                                ((const WaypointMapItem &)item).waypoint,
                                true, true);
    break;
  case MapItem::Type::TASK_OZ:
    dlgTargetShowModal(((const TaskOZMapItem &)item).index);
    break;
  case MapItem::Type::TRAFFIC:
    dlgFlarmTrafficDetailsShowModal(((const TrafficMapItem &)item).id);
    break;

#ifdef HAVE_NOAA
  case MapItem::Type::WEATHER:
    dlgNOAADetailsShowModal(((const WeatherStationMapItem &)item).station);
    break;
#endif

  case MapItem::Type::OVERLAY:
    ShowWeatherDialog(_T("overlay"));
    break;

  case MapItem::Type::RASP:
    ShowWeatherDialog(_T("rasp"));
    break;

 case MapItem::Type::ARRIVAL_TIME_RING:
   // This item type has no details view.
   break;
 }
}

void
ShowMapItemListDialog(const MapItemList &list,
                      const DialogLook &dialog_look,
                      const MapLook &look,
                      const TrafficLook &traffic_look,
                      const FinalGlideBarLook &final_glide_look,
                      const MapSettings &settings,
                      Waypoints *waypoints,
                      ProtectedAirspaceWarningManager *airspace_warnings)
{
  switch (list.size()) {
  case 0:
    /* no map items in the list */
    return;

  case 1:
    /* only one map item, show it */
    ShowMapItemDialog(*list[0], waypoints, airspace_warnings);
    break;

  default:
    /* more than one map item: show a list */

    int i = ShowMapItemListDialog(list, dialog_look, look,
                                  traffic_look, final_glide_look, settings);
    assert(i >= -1 && i < (int)list.size());
    if (i >= 0)
      ShowMapItemDialog(*list[i], waypoints, airspace_warnings);
  }
}

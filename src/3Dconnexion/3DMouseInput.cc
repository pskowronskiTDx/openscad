// -------------------------------------------------------------------------------------------------
// This source file is part of the OpenSCAD project.
//
// Copyright (c) 2014-2023 3Dconnexion.
//
// This source code is released under the GNU General Public License, (see "LICENSE").
// -------------------------------------------------------------------------------------------------

#include "3DMouseInput.h"
#include "degree_trig.h"
#include "MainWindow.h"
#include "QGLView.h"
#include "Renderer.h"

#include <QAction>
#include <QBuffer>
#include <QByteArray>
#include <QtCore/QCoreApplication>
#include <QIcon>

#include <algorithm>
#include <array>

constexpr double MIN_ZOOM = 1 ;
constexpr uint8_t LCD_ICON_SIZE = 24u;
constexpr uint8_t MATRIX_SIZE = 16u;

bool TDMouseInput::checkQGLView() const {
  if (m_p_parent_window == nullptr) {
    return false;
  }

  if (m_p_parent_window->qglview == nullptr) {
    return false;
  }

  return true;
}

TDMouseInput::TDMouseInput(MainWindow *p_parent_window, bool multi_threaded, bool row_major)
  : TDx::SpaceMouse::Navigation3D::CNavigation3D(multi_threaded, row_major),
  QObject(p_parent_window),
  m_p_parent_window(p_parent_window)
{
  if (checkQGLView()) {
    m_p_parent_window->qglview->setPivotIcon(":/icons/3dx_pivot.png");
  }
}

void TDMouseInput::initializeSampling()
{
  for (uint32_t i = 1; i < m_sampling_pattern.size(); i++) {
    const float coefficient =
    sqrt(static_cast<float>(i) / static_cast<float>(m_sampling_pattern.size()));
    const float angle = 2.4f * static_cast<float>(i);
    const float x = coefficient * sin(angle);
    const float y = coefficient * cos(angle);
    m_sampling_pattern.at(i)[0] = x;
    m_sampling_pattern.at(i)[1] = y;
  }
}

TDMouseInput::~TDMouseInput()
{
  try
  {
    disableNavigation();
  }
  catch(const std::system_error &sys_err)
  {
    std::cerr << sys_err.what() << '\n';
  }
}

bool TDMouseInput::enableNavigation()
{
  PutProfileHint("OpenSCAD");
  std::error_code error_code;
  EnableNavigation(true, error_code);

  if (error_code) {
    return false;
  }

  PutFrameTimingSource(TimingSource::SpaceMouse);
  initializeSampling();

  return true;
}

void TDMouseInput::disableNavigation()
{
  EnableNavigation(false);
}

void TDMouseInput::registerCommand(QAction *p_qaction)
{
  if (p_qaction == nullptr) {
    return;
  }

  Command new_command(p_qaction);
  m_id_to_command.try_emplace(p_qaction->objectName().toStdString(), new_command);

  return;
}

TDMouseInput::Command::Command(QAction *const p_qaction)
: m_p_qaction(p_qaction) {}

TDxCommand TDMouseInput::Command::toCCommand() const
{
  if (m_p_qaction == nullptr) {
    return TDxCommand("NULL", "NULL");
  }

  std::string id = m_p_qaction->objectName().toStdString();
  std::string name = QAction::tr(m_p_qaction->iconText().toStdString().c_str()).toStdString();
  std::string description = m_p_qaction->whatsThis().toStdString();
  description.append(m_p_qaction->toolTip().toStdString());

  return TDx::SpaceMouse::CCommand(id, name, description);
}

TDxImage TDMouseInput::Command::getCImage() const
{
  if (m_p_qaction == nullptr) {
    return TDxImage::FromFile("", 0, "NULL");
  }

  const QIcon iconImg = m_p_qaction->icon();

  if(iconImg.isNull()) {
    return TDxImage::FromFile("", 0, "NULL");
  }

  const QImage qimage = iconImg.pixmap(QSize(LCD_ICON_SIZE, LCD_ICON_SIZE)).toImage();
  QByteArray qbyteArray;
  QBuffer qbuffer(&qbyteArray);
  qimage.save(&qbuffer, "PNG");

  return TDxImage::FromData(qbyteArray.toStdString(), 0, m_p_qaction->objectName().toStdString().c_str());

}

void TDMouseInput::Command::run()
{
  if (m_p_qaction == nullptr) {
    return;
  }
  m_p_qaction->trigger();
  return;
}

void TDMouseInput::initializeCommandsMap()
{
  if(m_p_parent_window == nullptr) {
    return;
  }

  if(m_p_parent_window->animateWidget == nullptr) {
    return;
  }

  registerCommand(m_p_parent_window->editActionRedo);
  registerCommand(m_p_parent_window->editActionUndo);
  registerCommand(m_p_parent_window->editActionZoomTextIn);
  registerCommand(m_p_parent_window->editActionZoomTextOut);
  registerCommand(m_p_parent_window->editActionUnindent);
  registerCommand(m_p_parent_window->editActionIndent);
  registerCommand(m_p_parent_window->fileActionNew);
  registerCommand(m_p_parent_window->fileActionOpen);
  registerCommand(m_p_parent_window->fileActionSave);
  registerCommand(m_p_parent_window->designAction3DPrint);
  registerCommand(m_p_parent_window->designActionRender);
  registerCommand(m_p_parent_window->viewActionShowAxes);
  registerCommand(m_p_parent_window->viewActionShowEdges);
  registerCommand(m_p_parent_window->viewActionZoomIn);
  registerCommand(m_p_parent_window->viewActionZoomOut);
  registerCommand(m_p_parent_window->viewActionTop);
  registerCommand(m_p_parent_window->viewActionBottom);
  registerCommand(m_p_parent_window->viewActionLeft);
  registerCommand(m_p_parent_window->viewActionRight);
  registerCommand(m_p_parent_window->viewActionFront);
  registerCommand(m_p_parent_window->viewActionBack);
  registerCommand(m_p_parent_window->viewActionSurfaces);
  registerCommand(m_p_parent_window->viewActionWireframe);
  registerCommand(m_p_parent_window->viewActionShowCrosshairs);
  registerCommand(m_p_parent_window->viewActionThrownTogether);
  registerCommand(m_p_parent_window->viewActionPerspective);
  registerCommand(m_p_parent_window->viewActionOrthogonal);
  registerCommand(m_p_parent_window->designActionPreview);
  registerCommand(m_p_parent_window->fileActionExportSTL);
  registerCommand(m_p_parent_window->fileActionExportAMF);
  registerCommand(m_p_parent_window->fileActionExport3MF);
  registerCommand(m_p_parent_window->fileActionExportOFF);
  registerCommand(m_p_parent_window->fileActionExportWRL);
  registerCommand(m_p_parent_window->fileActionExportDXF);
  registerCommand(m_p_parent_window->fileActionExportSVG);
  registerCommand(m_p_parent_window->fileActionExportCSG);
  registerCommand(m_p_parent_window->fileActionExportPDF);
  registerCommand(m_p_parent_window->fileActionExportImage);
  registerCommand(m_p_parent_window->viewActionViewAll);
  registerCommand(m_p_parent_window->viewActionResetView);
  registerCommand(m_p_parent_window->viewActionShowScaleProportional);
  registerCommand(m_p_parent_window->fileActionNewWindow);
  registerCommand(m_p_parent_window->fileActionOpenWindow);
  registerCommand(m_p_parent_window->fileActionSaveAs);
  registerCommand(m_p_parent_window->fileActionSaveAll);
  registerCommand(m_p_parent_window->fileActionReload);
  registerCommand(m_p_parent_window->fileActionQuit);
  registerCommand(m_p_parent_window->editActionCut);
  registerCommand(m_p_parent_window->editActionCopy);
  registerCommand(m_p_parent_window->editActionPaste);
  registerCommand(m_p_parent_window->editActionComment);
  registerCommand(m_p_parent_window->editActionUncomment);
  registerCommand(m_p_parent_window->editActionNextTab);
  registerCommand(m_p_parent_window->editActionPrevTab);
  registerCommand(m_p_parent_window->editActionCopyViewport);
  registerCommand(m_p_parent_window->editActionCopyVPT);
  registerCommand(m_p_parent_window->editActionCopyVPR);
  registerCommand(m_p_parent_window->editActionCopyVPD);
  registerCommand(m_p_parent_window->editActionCopyVPF);
  registerCommand(m_p_parent_window->windowActionHideEditor);
  registerCommand(m_p_parent_window->designActionReloadAndPreview);
  registerCommand(m_p_parent_window->designActionAutoReload);
  registerCommand(m_p_parent_window->designCheckValidity);
  registerCommand(m_p_parent_window->designActionDisplayAST);
  registerCommand(m_p_parent_window->designActionDisplayCSGTree);
  registerCommand(m_p_parent_window->designActionDisplayCSGProducts);
  registerCommand(m_p_parent_window->viewActionPreview);
  registerCommand(m_p_parent_window->viewActionDiagonal);
  registerCommand(m_p_parent_window->viewActionCenter);
  registerCommand(m_p_parent_window->windowActionHideConsole);
  registerCommand(m_p_parent_window->helpActionAbout);
  registerCommand(m_p_parent_window->helpActionOfflineManual);
  registerCommand(m_p_parent_window->helpActionOfflineCheatSheet);
  registerCommand(m_p_parent_window->fileActionClearRecent);
  registerCommand(m_p_parent_window->fileActionClose);
  registerCommand(m_p_parent_window->editActionPreferences);
  registerCommand(m_p_parent_window->editActionFind);
  registerCommand(m_p_parent_window->editActionFindAndReplace);
  registerCommand(m_p_parent_window->editActionFindNext);
  registerCommand(m_p_parent_window->editActionFindPrevious);
  registerCommand(m_p_parent_window->editActionUseSelectionForFind);
  registerCommand(m_p_parent_window->editActionJumpToNextError);
  registerCommand(m_p_parent_window->designActionFlushCaches);
  registerCommand(m_p_parent_window->helpActionHomepage);
  registerCommand(m_p_parent_window->helpActionLibraryInfo);
  registerCommand(m_p_parent_window->fileShowLibraryFolder);
  registerCommand(m_p_parent_window->helpActionFontInfo);
  registerCommand(m_p_parent_window->editActionConvertTabsToSpaces);
  registerCommand(m_p_parent_window->editActionToggleBookmark);
  registerCommand(m_p_parent_window->editActionNextBookmark);
  registerCommand(m_p_parent_window->editActionPrevBookmark);
  registerCommand(m_p_parent_window->viewActionHideEditorToolBar);
  registerCommand(m_p_parent_window->helpActionCheatSheet);
  registerCommand(m_p_parent_window->windowActionHideCustomizer);
  registerCommand(m_p_parent_window->viewActionHide3DViewToolBar);
  registerCommand(m_p_parent_window->windowActionHideErrorLog);
  registerCommand(m_p_parent_window->windowActionSelectEditor);
  registerCommand(m_p_parent_window->windowActionSelectConsole);
  registerCommand(m_p_parent_window->windowActionSelectCustomizer);
  registerCommand(m_p_parent_window->windowActionSelectErrorLog);
  registerCommand(m_p_parent_window->windowActionNextWindow);
  registerCommand(m_p_parent_window->windowActionPreviousWindow);
  registerCommand(m_p_parent_window->editActionInsertTemplate);
  registerCommand(m_p_parent_window->helpActionManual);

  auto animate_actions = m_p_parent_window->animateWidget->actions();

  registerCommand(animate_actions[0]);
  registerCommand(animate_actions[1]);
  registerCommand(animate_actions[2]);
  registerCommand(animate_actions[3]);
  registerCommand(animate_actions[4]);
  registerCommand(animate_actions[5]);
  registerCommand(animate_actions[6]);
}

TDxCategory TDMouseInput::getAnimateCategory() const
{
  TDxCategory result("Animate", "Animate");

  const std::array<std::string, 7> animate_labels {
    "Toggle pause",
    "Start",
    "Step backward",
    "Play",
    "Pause",
    "Step forward",
    "End"};

  if (m_p_parent_window == nullptr) {
    return TDxCategory("NULL", "NULL");
  }

  if (m_p_parent_window->animateWidget == nullptr) {
    return TDxCategory("NULL", "NULL");
  }

  auto animate_actions = m_p_parent_window->animateWidget->actions();
  uint32_t itr = 0u;

  for (const QAction *action : animate_actions) {
    if (action == nullptr) {
      itr++;
      continue;
    }

    std::string id = action->objectName().toStdString();

    if (m_id_to_command.find(id) == m_id_to_command.end()) {
      itr++;
      continue;
    }

    TDxCommand ccommand = m_id_to_command.at(id).toCCommand();
    ccommand.PutLabel(animate_labels[itr]);
    result.push_back(std::move(ccommand));
    itr++;
  }
  return result;
}

void TDMouseInput::exportCommands()
{
  if (m_p_parent_window == nullptr) {
    return;
  }

  initializeCommandsMap();

  using TDx::SpaceMouse::CCommandSet;
  CCommandSet menu_bar("Default", "OpenSCAD");

  std::vector<QMenu *> p_qmenus = {
    m_p_parent_window->menu_File,
    m_p_parent_window->menuOpenRecent,
    m_p_parent_window->menuExamples,
    m_p_parent_window->menuExport,
    m_p_parent_window->menu_Edit,
    m_p_parent_window->menu_Design,
    m_p_parent_window->menu_View,
    m_p_parent_window->menuHelp,
    m_p_parent_window->menuWindow};

  for (const QMenu *p_qmenu : p_qmenus) {
    if (p_qmenu == nullptr) {
      continue;
    }

    std::string title = p_qmenu->title().toStdString();
    title.erase(std::remove(title.begin(), title.end(), '&'), title.end());
    TDx::SpaceMouse::CCategory menu(title, title);

    for (const QAction *p_qaction : p_qmenu->actions()) {
      if (p_qaction == nullptr) {
        continue;
      }

      std::string id = p_qaction->objectName().toStdString();
      if (m_id_to_command.find(id) != m_id_to_command.end()) {
        menu.push_back(m_id_to_command[id].toCCommand());
      }
    }
    menu_bar.push_back(std::move(menu));
  }

  menu_bar.push_back(getAnimateCategory());

  std::vector<TDx::CImage> images;

  for (const auto &[key, command] : m_id_to_command) {
    if (!command.getCImage().empty()) {
      images.push_back(command.getCImage());
    }
  }

  AddCommandSet(menu_bar);
  PutActiveCommands(menu_bar.GetId());
  AddImages(images);

  return;
}

long TDMouseInput::GetCoordinateSystem(navlib::matrix_t &matrix) const
{
  std::copy_n(Eigen::Matrix4d::Identity().eval().data(), MATRIX_SIZE, matrix.begin());
  return 0;
}

long TDMouseInput::GetCameraMatrix(navlib::matrix_t &affine) const
{
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  std::copy_n(m_p_parent_window->qglview->cam.getAffine().data(), MATRIX_SIZE, affine.begin());

  return 0;
}

long TDMouseInput::SetCameraMatrix(const navlib::matrix_t &affine)
{
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  auto newAffine = Eigen::Matrix4d(&affine.m00);
  auto buffer = m_p_parent_window->qglview->cam;
  buffer.setAffine(newAffine);

  for (uint8_t i = 0; i < MATRIX_SIZE; i++) {
    if (std::isnan(buffer.getAffine().data()[i])) {
      return navlib::make_result_code(navlib::navlib_errc::invalid_argument);
    }
  }

  m_p_parent_window->qglview->applyAffine(newAffine);

  return 0;
}

long TDMouseInput::GetIsViewPerspective(navlib::bool_t &p) const
{
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  p = m_p_parent_window->qglview->cam.getProjection() == Camera::ProjectionType::PERSPECTIVE;

  return 0;
}

long TDMouseInput::GetViewFOV(double &fov) const
{
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  fov = deg2rad(m_p_parent_window->qglview->cam.fovValue());

  return 0;
}

long TDMouseInput::SetViewFOV(double fov)
{
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  m_p_parent_window->qglview->cam.setVpf(rad2deg(fov));

  return 0;
}

long TDMouseInput::GetModelExtents(navlib::box_t &nav_box) const
{
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  if (m_p_parent_window->qglview->renderer == nullptr) {
    nav_box = {-10.0, -10.0, -10.0, 10.0, 10.0, 10.0};
    return 0;
  }

  BoundingBox box = m_p_parent_window->qglview->renderer->getBoundingBox();

  if (box.isEmpty() || box.isNull()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  std::copy_n(box.min().data(), box.min().size(), &nav_box.min.x);
  std::copy_n(box.max().data(), box.max().size(), &nav_box.max.x);

  return 0;
}

long TDMouseInput::GetViewExtents(navlib::box_t &bounding_box) const
{
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  if (m_p_parent_window->qglview->cam.getProjection() == Camera::ProjectionType::PERSPECTIVE) {
    return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
  }

  const auto frustum = m_p_parent_window->qglview->cam.getFrustum();
  bounding_box = {frustum.left, frustum.bottom, frustum.nearVal, frustum.right, frustum.top, frustum.farVal};

  return 0;
}

long TDMouseInput::SetViewExtents(const navlib::box_t &bounding_box)
{
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  if (m_p_parent_window->qglview->cam.getProjection() == Camera::ProjectionType::PERSPECTIVE) {
    return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
  }

  const auto frustum = m_p_parent_window->qglview->cam.getFrustum();
  m_p_parent_window->qglview->cam.scaleDistance(
    (bounding_box.max.y - bounding_box.min.y) / (frustum.top - frustum.bottom));

  return 0;
}

long TDMouseInput::GetViewFrustum(navlib::frustum_t &f) const
{
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  if (m_p_parent_window->qglview->cam.getProjection() != Camera::ProjectionType::PERSPECTIVE) {
    return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
  }

  const auto frustum = m_p_parent_window->qglview->cam.getFrustum();
  // Set a fixed nearVal to work around the navlib assuming the near is fixed.
  const double nf = 0.01 / frustum.nearVal;
  f = {frustum.left * nf, frustum.right * nf, frustum.bottom * nf,
       frustum.top * nf, frustum.nearVal * nf, frustum.farVal};

  return 0;
}

long TDMouseInput::SetViewFrustum(const navlib::frustum_t &)
{
  return navlib::make_result_code(navlib::navlib_errc::function_not_supported);
}

long TDMouseInput::GetUnitsToMeters(double &factor) const
{
  factor = 0.1;
  return 0;
}

long TDMouseInput::GetIsSelectionEmpty(navlib::bool_t &s) const
{
  s = true;
  return 0;
}

long TDMouseInput::SetActiveCommand(std::string cmd)
{
  if (m_id_to_command.find(cmd) != m_id_to_command.end()) {
    m_id_to_command[cmd].run();
  }

  return 0;
}

long TDMouseInput::GetSelectionTransform(navlib::matrix_t &) const
{
  return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long TDMouseInput::GetSelectionExtents(navlib::box_t &) const
{
  return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long TDMouseInput::SetSelectionTransform(const navlib::matrix_t &)
{
  return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long TDMouseInput::GetFrontView(navlib::matrix_t &matrix) const
{
  matrix = { 1.0, 0.0, 0.0, 0.0,
             0.0, 0.0, 1.0, 0.0,
             0.0,-1.0, 0.0, 0.0,
             0.0, 0.0, 0.0, 1.0 };
  return 0;
};
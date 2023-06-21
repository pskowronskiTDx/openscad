/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2015 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <QtCore/QCoreApplication>
#include <QAction>
#include "MainWindow.h"
#include "3DMouseInput.h"
#include "QGLView.h"
#include "degree_trig.h"

#include <algorithm>
#include <array>

constexpr double MIN_ZOOM = 1 ;
constexpr std::size_t MATRIX_SIZE = 16 * sizeof(double);

inline double rad2deg(double x)
{
	return x * M_RAD2DEG;
}

inline double deg2rad(double x)
{
	return x * M_DEG2RAD;
}

inline bool TDMouseInput::checkQGLView() const {
	if (m_p_main_window == nullptr) {
		return false;
	}

	if (m_p_main_window->qglview == nullptr) {
		return false;
	}

	return true;
}

TDMouseInput::TDMouseInput(MainWindow *const p_window, bool multi_threaded, bool row_major)
	: TDx::SpaceMouse::Navigation3D::CNavigation3D(multi_threaded, row_major),
	  m_p_main_window(p_window)
{
	QString pivot_icon_path = QCoreApplication::applicationDirPath();
	pivot_icon_path.append("/resources/icons/3dx_pivot.png");

	if (checkQGLView()) {
		m_p_main_window->qglview->setPivotIcon(pivot_icon_path);
	}
}

void TDMouseInput::initializeSampling()
{
	m_sampling_pattern.resize(sampleCount);

	if (sampleCount > 0) {
		m_sampling_pattern.at(0)[0] = 0.0;
		m_sampling_pattern.at(0)[1] = 0.0;
	}

	for (uint32_t i = 1; i < sampleCount; i++) {
		float coefficient =
			sqrt(static_cast<float>(i) / static_cast<float>(sampleCount));
		float angle = 2.4f * static_cast<float>(i);
		float x = coefficient * sin(angle);
		float y = coefficient * cos(angle);
		m_sampling_pattern.at(i)[0] = x;
		m_sampling_pattern.at(i)[1] = y;
	}

	return;
}

TDMouseInput::~TDMouseInput()
{
	disableNavigation();
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

void TDMouseInput::registerCommand(QAction *p_qaction,	const std::string &icon_file_name = "") 
{
	if (p_qaction == nullptr) {
		return;
	}

	std::string icon_file_path;
	if (!icon_file_name.empty()) {
		icon_file_path = QCoreApplication::applicationDirPath().toStdString().append("/resources/icons/");
		icon_file_path.append(icon_file_name);
	}

	Command new_command(p_qaction, icon_file_path);
	m_id_to_command.emplace(p_qaction->objectName().toStdString(), new_command);

	return;
}

TDMouseInput::Command::Command(QAction *const p_qaction, const std::string &icon_file_path)
: m_p_qaction(p_qaction),
  m_icon_path(icon_file_path) {}

TDxCommand TDMouseInput::Command::toCCommand() const
{
	if (m_p_qaction == nullptr) {
		return TDxCommand("NULL", "NULL");
	}

	std::string id = m_p_qaction->objectName().toStdString();
	std::string name = m_p_qaction->tr(m_p_qaction->iconText().toStdString().c_str()).toStdString();
	std::string description = m_p_qaction->whatsThis().toStdString();
	description.append(m_p_qaction->toolTip().toStdString());

	return TDx::SpaceMouse::CCommand(id, name, description);
}

TDxImage TDMouseInput::Command::getCImage() const
{
	if (m_icon_path.empty() || (m_p_qaction == nullptr)) {
		return TDxImage::FromFile("", 0, "NULL");
	}
	return TDxImage::FromFile(m_icon_path, 0, m_p_qaction->objectName().toStdString().c_str());
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
	if(m_p_main_window == nullptr) {
		return;
	}

	if(m_p_main_window->animateWidget == nullptr) {
		return;
	}

	registerCommand(m_p_main_window->editActionRedo, "redo.svg");
	registerCommand(m_p_main_window->editActionUndo, "undo.svg");
	registerCommand(m_p_main_window->editActionZoomTextIn, "zoom-text-in.svg");
	registerCommand(m_p_main_window->editActionZoomTextOut, "zoom-text-out.svg");
	registerCommand(m_p_main_window->editActionUnindent, "unindent.svg");
	registerCommand(m_p_main_window->editActionIndent, "indent.svg");
	registerCommand(m_p_main_window->fileActionNew, "new.svg");
	registerCommand(m_p_main_window->fileActionOpen, "open.svg");
	registerCommand(m_p_main_window->fileActionSave, "save.svg");
	registerCommand(m_p_main_window->designAction3DPrint, "send.svg");
	registerCommand(m_p_main_window->designActionRender, "render.svg");
	registerCommand(m_p_main_window->viewActionShowAxes, "axes.svg");
	registerCommand(m_p_main_window->viewActionShowEdges, "show-edges.svg");
	registerCommand(m_p_main_window->viewActionZoomIn, "zoom-in.svg");
	registerCommand(m_p_main_window->viewActionZoomOut, "zoom-out.svg");
	registerCommand(m_p_main_window->viewActionTop, "view-top.svg");
	registerCommand(m_p_main_window->viewActionBottom, "view-bottom.svg");
	registerCommand(m_p_main_window->viewActionLeft, "view-left.svg");
	registerCommand(m_p_main_window->viewActionRight, "view-right.svg");
	registerCommand(m_p_main_window->viewActionFront, "view-front.svg");
	registerCommand(m_p_main_window->viewActionBack, "view-back.svg");
	registerCommand(m_p_main_window->viewActionSurfaces, "surface.svg");
	registerCommand(m_p_main_window->viewActionWireframe, "wireframe.svg");
	registerCommand(m_p_main_window->viewActionShowCrosshairs, "crosshairs.svg");
	registerCommand(m_p_main_window->viewActionThrownTogether, "throwntogether.svg");
	registerCommand(m_p_main_window->viewActionPerspective, "perspective.svg");
	registerCommand(m_p_main_window->viewActionOrthogonal, "orthogonal.svg");
	registerCommand(m_p_main_window->designActionPreview, "preview.svg");
	registerCommand(m_p_main_window->fileActionExportSTL, "export-stl.svg");
	registerCommand(m_p_main_window->fileActionExportAMF, "export-amf.svg");
	registerCommand(m_p_main_window->fileActionExport3MF, "export-3mf.svg");
	registerCommand(m_p_main_window->fileActionExportOFF, "export-off.svg");
	registerCommand(m_p_main_window->fileActionExportWRL, "export-wrl.svg");
	registerCommand(m_p_main_window->fileActionExportDXF, "export-dxf.svg");
	registerCommand(m_p_main_window->fileActionExportSVG, "export-svg.svg");
	registerCommand(m_p_main_window->fileActionExportCSG, "export-csg.svg");
	registerCommand(m_p_main_window->fileActionExportPDF, "export-pdf.svg");
	registerCommand(m_p_main_window->fileActionExportImage, "export-png.svg");
	registerCommand(m_p_main_window->viewActionViewAll, "zoom-all.svg");
	registerCommand(m_p_main_window->viewActionResetView, "reset-view.svg");
	registerCommand(m_p_main_window->viewActionShowScaleProportional, "scalemarkers.svg");
	registerCommand(m_p_main_window->fileActionNewWindow);
	registerCommand(m_p_main_window->fileActionOpenWindow);
	registerCommand(m_p_main_window->fileActionSaveAs);
	registerCommand(m_p_main_window->fileActionSaveAll);
	registerCommand(m_p_main_window->fileActionReload);
	registerCommand(m_p_main_window->fileActionQuit);
	registerCommand(m_p_main_window->editActionCut);
	registerCommand(m_p_main_window->editActionCopy);
	registerCommand(m_p_main_window->editActionPaste);
	registerCommand(m_p_main_window->editActionComment);
	registerCommand(m_p_main_window->editActionUncomment);
	registerCommand(m_p_main_window->editActionNextTab);
	registerCommand(m_p_main_window->editActionPrevTab);
	registerCommand(m_p_main_window->editActionCopyViewport);
	registerCommand(m_p_main_window->editActionCopyVPT);
	registerCommand(m_p_main_window->editActionCopyVPR);
	registerCommand(m_p_main_window->editActionCopyVPD);
	registerCommand(m_p_main_window->editActionCopyVPF);
	registerCommand(m_p_main_window->windowActionHideEditor);
	registerCommand(m_p_main_window->designActionReloadAndPreview);
	registerCommand(m_p_main_window->designActionAutoReload);
	registerCommand(m_p_main_window->designCheckValidity);
	registerCommand(m_p_main_window->designActionDisplayAST);
	registerCommand(m_p_main_window->designActionDisplayCSGTree);
	registerCommand(m_p_main_window->designActionDisplayCSGProducts);
	registerCommand(m_p_main_window->viewActionPreview);
	registerCommand(m_p_main_window->viewActionDiagonal);
	registerCommand(m_p_main_window->viewActionCenter);
	registerCommand(m_p_main_window->windowActionHideConsole);
	registerCommand(m_p_main_window->helpActionAbout);
	registerCommand(m_p_main_window->helpActionOfflineManual);
	registerCommand(m_p_main_window->helpActionOfflineCheatSheet);
	registerCommand(m_p_main_window->fileActionClearRecent);
	registerCommand(m_p_main_window->fileActionClose);
	registerCommand(m_p_main_window->editActionPreferences);
	registerCommand(m_p_main_window->editActionFind);
	registerCommand(m_p_main_window->editActionFindAndReplace);
	registerCommand(m_p_main_window->editActionFindNext);
	registerCommand(m_p_main_window->editActionFindPrevious);
	registerCommand(m_p_main_window->editActionUseSelectionForFind);
	registerCommand(m_p_main_window->editActionJumpToNextError);
	registerCommand(m_p_main_window->designActionFlushCaches);
	registerCommand(m_p_main_window->helpActionHomepage);
	registerCommand(m_p_main_window->helpActionLibraryInfo);
	registerCommand(m_p_main_window->fileShowLibraryFolder);
	registerCommand(m_p_main_window->helpActionFontInfo);
	registerCommand(m_p_main_window->editActionConvertTabsToSpaces);
	registerCommand(m_p_main_window->editActionToggleBookmark);
	registerCommand(m_p_main_window->editActionNextBookmark);
	registerCommand(m_p_main_window->editActionPrevBookmark);
	registerCommand(m_p_main_window->viewActionHideEditorToolBar);
	registerCommand(m_p_main_window->helpActionCheatSheet);
	registerCommand(m_p_main_window->windowActionHideCustomizer);
	registerCommand(m_p_main_window->viewActionHide3DViewToolBar);
	registerCommand(m_p_main_window->windowActionHideErrorLog);
	registerCommand(m_p_main_window->windowActionSelectEditor);
	registerCommand(m_p_main_window->windowActionSelectConsole);
	registerCommand(m_p_main_window->windowActionSelectCustomizer);
	registerCommand(m_p_main_window->windowActionSelectErrorLog);
	registerCommand(m_p_main_window->windowActionNextWindow);
	registerCommand(m_p_main_window->windowActionPreviousWindow);
	registerCommand(m_p_main_window->editActionInsertTemplate);
	registerCommand(m_p_main_window->helpActionManual);

	auto animate_actions = m_p_main_window->animateWidget->actions();

	registerCommand(animate_actions[0], "animate.svg");
	registerCommand(animate_actions[1], "vcr-control-start.svg");
	registerCommand(animate_actions[2], "vcr-control-step-back.svg");
	registerCommand(animate_actions[3], "vcr-control-play.svg");
	registerCommand(animate_actions[4], "vcr-control-pause.svg");
	registerCommand(animate_actions[5], "vcr-control-step-forward.svg");
	registerCommand(animate_actions[6], "vcr-control-end.svg");
}

TDxCategory TDMouseInput::getAnimateCategory()
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

	if (m_p_main_window == nullptr) {
		return TDxCategory("NULL", "NULL");
	}

	if (m_p_main_window->animateWidget == nullptr) {
		return TDxCategory("NULL", "NULL");
	}

	auto animate_actions = m_p_main_window->animateWidget->actions();

	for (uint32_t itr = 0u; QAction * action : animate_actions) {
		if (action == nullptr) {
			itr++;
			continue;
		}

		std::string id = action->objectName().toStdString();

		if (m_id_to_command.find(id) == m_id_to_command.end()) {
			itr++;
			continue;
		}

		TDxCommand ccommand = m_id_to_command[id].toCCommand();
		ccommand.PutLabel(animate_labels[itr]);
		result.push_back(std::move(ccommand));
		itr++;
	}
	return result;
}

void TDMouseInput::exportCommands()
{
	if (m_p_main_window == nullptr) {
		return;
	}

	initializeCommandsMap();

	using TDx::SpaceMouse::CCommandSet;
	CCommandSet menu_bar("Default", "OpenSCAD");

	std::vector<QMenu *> p_qmenus = {
		m_p_main_window->menu_File,
		m_p_main_window->menuOpenRecent,
		m_p_main_window->menuExamples,
		m_p_main_window->menuExport,
		m_p_main_window->menu_Edit,
		m_p_main_window->menu_Design,
		m_p_main_window->menu_View,
		m_p_main_window->menuHelp,
		m_p_main_window->menuWindow};

	for (QMenu *p_qmenu : p_qmenus) {
		if (p_qmenu == nullptr) {
			continue;
		}

		std::string title = p_qmenu->title().toStdString();
		title.erase(std::remove(title.begin(), title.end(), '&'), title.end());
		TDx::SpaceMouse::CCategory menu(title, title);

		for (QAction *p_qaction : p_qmenu->actions()) {

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

	for (auto &entry : m_id_to_command) {
		if (!entry.second.getCImage().empty()) {
			images.push_back(entry.second.getCImage());
		}
	}

	AddCommandSet(menu_bar);
	PutActiveCommands(menu_bar.GetId());
	AddImages(images);

	return;
}

long TDMouseInput::GetCoordinateSystem(navlib::matrix_t &matrix) const
{
	matrix.m00 = 1.0;
	std::memcpy(&matrix.m00, Eigen::Matrix4d::Identity().eval().data(), MATRIX_SIZE);
	return 0;
}

long TDMouseInput::GetCameraMatrix(navlib::matrix_t &affine) const
{
	if(!checkQGLView()) {
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}

	std::memcpy(&affine.m00, m_p_main_window->qglview->cam.getAffine().data(), MATRIX_SIZE);
	return 0;
}

long TDMouseInput::SetCameraMatrix(const navlib::matrix_t &affine)
{
	if(!checkQGLView()) {
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}

	auto newAffine = Eigen::Matrix4d(&affine.m00);
	auto buffer = m_p_main_window->qglview->cam;
	buffer.setAffine(newAffine);

	for (uint8_t i = 0; i < 16; i++) {
		if (std::isnan(buffer.getAffine().data()[i])) {
			return navlib::make_result_code(navlib::navlib_errc::invalid_argument);
		}
	}

	m_p_main_window->qglview->applyAffine(newAffine);
	return 0;
}

long TDMouseInput::GetIsViewPerspective(navlib::bool_t &p) const
{
	if(!checkQGLView()) {
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}

	p = m_p_main_window->qglview->cam.getProjection() == Camera::ProjectionType::PERSPECTIVE;
	return 0;
}

long TDMouseInput::GetViewFOV(double &fov) const
{
	if(!checkQGLView()) {
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}

	fov = deg2rad(m_p_main_window->qglview->cam.fovValue());
	return 0;
}

long TDMouseInput::SetViewFOV(double fov)
{
	if(!checkQGLView()) {
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}

	m_p_main_window->qglview->cam.setVpf(rad2deg(fov));
	return 0;
}

long TDMouseInput::GetModelExtents(navlib::box_t &nav_box) const
{
	if(!checkQGLView()) {
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}

	if (m_p_main_window->qglview->renderer == nullptr) {
		nav_box = {-10.0, -10.0, -10.0, 10.0, 10.0, 10.0};
		return 0;
	}

	BoundingBox box = m_p_main_window->qglview->renderer->getBoundingBox();
	
	if (box.isEmpty() || box.isNull()) {
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}

	std::memcpy(&nav_box.min.x, box.min().data(), 3 * sizeof(double));
	std::memcpy(&nav_box.max.x, box.max().data(), 3 * sizeof(double));
	return 0;
}

long TDMouseInput::GetViewExtents(navlib::box_t &bounding_box) const
{
	if(!checkQGLView()) {
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}

	if (m_p_main_window->qglview->cam.getProjection() == Camera::ProjectionType::PERSPECTIVE) {
		return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
	}

	const auto frustum = m_p_main_window->qglview->cam.getFrustum();
	bounding_box = {frustum.left, frustum.bottom, frustum.nearVal, frustum.right, frustum.top, frustum.farVal};

	return 0;
}

long TDMouseInput::SetViewExtents(const navlib::box_t &bounding_box)
{
	if(!checkQGLView()) {
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}

	if (m_p_main_window->qglview->cam.getProjection() == Camera::ProjectionType::PERSPECTIVE) {
		return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
	}

	const auto frustum = m_p_main_window->qglview->cam.getFrustum();
	m_p_main_window->qglview->cam.scaleDistance((bounding_box.max.y - bounding_box.min.y) / (frustum.top - frustum.bottom));

	return 0;
}

long TDMouseInput::GetViewFrustum(navlib::frustum_t &f) const
{
	if(!checkQGLView()) {
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}

	if (m_p_main_window->qglview->cam.getProjection() != Camera::ProjectionType::PERSPECTIVE) {
		return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
	}

	const auto frustum = m_p_main_window->qglview->cam.getFrustum();
	// Set a fixed nearVal to work around the navlib assuming the near is fixed.
	const double nf = 0.01 / frustum.nearVal;
	f = {frustum.left * nf, frustum.right * nf, frustum.bottom * nf,
		 frustum.top * nf, frustum.nearVal * nf, frustum.farVal};

	return 0;
}

long TDMouseInput::SetViewFrustum(const navlib::frustum_t &f)
{
	return navlib::make_result_code(navlib::navlib_errc::function_not_supported);
}

long TDMouseInput::GetFrontView(navlib::matrix_t &matrix) const
{
	std::memcpy(&matrix.m00, Eigen::Matrix4d::Identity().eval().data(), MATRIX_SIZE);
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
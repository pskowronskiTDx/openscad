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

#include "3DMouseCmd.h"

#include "MainWindow.h"
#include <QTemporaryFile>
#include <QBuffer>
#include <QResource>

template<class T>
std::string ptrToStr(T* a){
	const void * address = static_cast<const void*>(a);
	std::stringstream ss;
	ss << address;  
	return ss.str(); 
}

QActionCommand::QActionCommand(QAction *action, std::string const &rpath)
	: qaction_(action), rpath_(rpath)
{
}

TDx::CImage QActionCommand::GetCImage() const
{
	QResource open_res(QString::fromStdString(rpath_));
	QByteArray open_res_arr(reinterpret_cast<const char *>(open_res.data()), int(open_res.size()));
	return TDx::CImage::FromData(open_res_arr.toStdString(), 0, ptrToStr(qaction_).c_str());
}

bool QActionCommand::HasImage() const
{
	return rpath_ != "";
}

std::string QActionCommand::Text() const
{
	auto str = qaction_->tr(qaction_->iconText().toStdString().c_str()).toStdString();
	return str;
}
std::string QActionCommand::ToolTips() const
{
	return qaction_->toolTip().toStdString();
}

std::string QActionCommand::Description() const
{
	return qaction_->whatsThis().toStdString();
}

TDx::SpaceMouse::CCommand QActionCommand::MakeCommand() const
{
	return TDx::SpaceMouse::CCommand(ptrToStr(qaction_), Text(), Description());
}

void QActionCommand::Run()
{
	qaction_->trigger();
}



void AddQAction(std::unordered_map<std::string, std::shared_ptr<QActionCommand>> &map,
								std::vector<std::shared_ptr<QActionCommand>> &v, QAction *a,
								std::string const &image = "")
{
	// defaut path for qt resource
	static const std::string base = ":/resources/icons/svg-default/";
	// no already added
	assert(map.find(ptrToStr(a)) == map.end()); 
	if (image == "") {
		v.push_back(std::make_shared<QActionCommand>(a, ""));
	}
	else {
		v.push_back(std::make_shared<QActionCommand>(a, base + image));
	}
	map[ptrToStr(a)] = v.back();
}

void QActionsHandler::ExportApplicationCmds(TDx::SpaceMouse::Navigation3D::CNavigation3D *nav)
{
	cmds_.reserve(110);
	// link widget to its description and optionally an image
	AddQAction(idToAction_, cmds_, win_->editActionRedo, "redo.svg");
	AddQAction(idToAction_, cmds_, win_->editActionUndo, "undo.svg");
	AddQAction(idToAction_, cmds_, win_->editActionZoomTextIn, "zoom-text-in.svg");
	AddQAction(idToAction_, cmds_, win_->editActionZoomTextOut, "zoom-text-out.svg");
	AddQAction(idToAction_, cmds_, win_->editActionUnindent, "unindent.svg");
	AddQAction(idToAction_, cmds_, win_->editActionIndent, "indent.svg");
	AddQAction(idToAction_, cmds_, win_->fileActionNew, "new.svg");
	AddQAction(idToAction_, cmds_, win_->fileActionOpen, "open.svg");
	AddQAction(idToAction_, cmds_, win_->fileActionSave, "save.svg");
	AddQAction(idToAction_, cmds_, win_->designAction3DPrint, "send.svg");
	AddQAction(idToAction_, cmds_, win_->designActionRender, "render.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionShowAxes, "axes.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionShowEdges, "show-edges.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionZoomIn, "zoom-in.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionZoomOut, "zoom-out.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionTop, "view-top.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionBottom, "view-bottom.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionLeft, "view-left.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionRight, "view-right.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionFront, "view-front.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionBack, "view-back.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionSurfaces, "surface.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionWireframe, "wireframe.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionShowCrosshairs, "crosshairs.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionThrownTogether, "throwntogether.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionPerspective, "perspective.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionOrthogonal, "orthogonal.svg");
	AddQAction(idToAction_, cmds_, win_->designActionPreview, "preview.svg");
	//AddQAction(idToAction_, cmds_, win_->viewActionAnimate, "animate.svg");
	AddQAction(idToAction_, cmds_, win_->fileActionExportSTL, "export-stl.svg");
	AddQAction(idToAction_, cmds_, win_->fileActionExportAMF, "export-amf.svg");
	AddQAction(idToAction_, cmds_, win_->fileActionExport3MF, "export-3mf.svg");
	AddQAction(idToAction_, cmds_, win_->fileActionExportOFF, "export-off.svg");
	AddQAction(idToAction_, cmds_, win_->fileActionExportWRL, "export-wrl.svg");
	AddQAction(idToAction_, cmds_, win_->fileActionExportDXF, "export-dxf.svg");
	AddQAction(idToAction_, cmds_, win_->fileActionExportSVG, "export-svg.svg");
	AddQAction(idToAction_, cmds_, win_->fileActionExportCSG, "export-csg.svg");
	AddQAction(idToAction_, cmds_, win_->fileActionExportPDF, "export-pdf.svg");
	AddQAction(idToAction_, cmds_, win_->fileActionExportImage, "export-png.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionViewAll, "zoom-all.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionResetView, "reset-view.svg");
	AddQAction(idToAction_, cmds_, win_->viewActionShowScaleProportional, "scalemarkers.svg");
	AddQAction(idToAction_, cmds_, win_->fileActionNewWindow);
	AddQAction(idToAction_, cmds_, win_->fileActionOpenWindow);
	AddQAction(idToAction_, cmds_, win_->fileActionSaveAs);
	AddQAction(idToAction_, cmds_, win_->fileActionSaveAll);
	AddQAction(idToAction_, cmds_, win_->fileActionReload);
	AddQAction(idToAction_, cmds_, win_->fileActionQuit);
	AddQAction(idToAction_, cmds_, win_->editActionCut);
	AddQAction(idToAction_, cmds_, win_->editActionCopy);
	AddQAction(idToAction_, cmds_, win_->editActionPaste);
	AddQAction(idToAction_, cmds_, win_->editActionComment);
	AddQAction(idToAction_, cmds_, win_->editActionUncomment);
	AddQAction(idToAction_, cmds_, win_->editActionNextTab);
	AddQAction(idToAction_, cmds_, win_->editActionPrevTab);
	AddQAction(idToAction_, cmds_, win_->editActionCopyViewport);
	AddQAction(idToAction_, cmds_, win_->editActionCopyVPT);
	AddQAction(idToAction_, cmds_, win_->editActionCopyVPR);
	AddQAction(idToAction_, cmds_, win_->editActionCopyVPD);
	AddQAction(idToAction_, cmds_, win_->editActionCopyVPF);
	AddQAction(idToAction_, cmds_, win_->windowActionHideEditor);
	AddQAction(idToAction_, cmds_, win_->designActionReloadAndPreview);
	AddQAction(idToAction_, cmds_, win_->designActionAutoReload);
	AddQAction(idToAction_, cmds_, win_->designCheckValidity);
	AddQAction(idToAction_, cmds_, win_->designActionDisplayAST);
	AddQAction(idToAction_, cmds_, win_->designActionDisplayCSGTree);
	AddQAction(idToAction_, cmds_, win_->designActionDisplayCSGProducts);
	AddQAction(idToAction_, cmds_, win_->viewActionPreview);
	AddQAction(idToAction_, cmds_, win_->viewActionDiagonal);
	AddQAction(idToAction_, cmds_, win_->viewActionCenter);
	AddQAction(idToAction_, cmds_, win_->windowActionHideConsole);
	AddQAction(idToAction_, cmds_, win_->helpActionAbout);
	AddQAction(idToAction_, cmds_, win_->helpActionOfflineManual);
	AddQAction(idToAction_, cmds_, win_->helpActionOfflineCheatSheet);
	AddQAction(idToAction_, cmds_, win_->fileActionClearRecent);
	AddQAction(idToAction_, cmds_, win_->fileActionClose);
	AddQAction(idToAction_, cmds_, win_->editActionPreferences);
	AddQAction(idToAction_, cmds_, win_->editActionFind);
	AddQAction(idToAction_, cmds_, win_->editActionFindAndReplace);
	AddQAction(idToAction_, cmds_, win_->editActionFindNext);
	AddQAction(idToAction_, cmds_, win_->editActionFindPrevious);
	AddQAction(idToAction_, cmds_, win_->editActionUseSelectionForFind);
	AddQAction(idToAction_, cmds_, win_->editActionJumpToNextError);
	AddQAction(idToAction_, cmds_, win_->designActionFlushCaches);
	AddQAction(idToAction_, cmds_, win_->helpActionHomepage);
	AddQAction(idToAction_, cmds_, win_->helpActionLibraryInfo);
	AddQAction(idToAction_, cmds_, win_->fileShowLibraryFolder);
	AddQAction(idToAction_, cmds_, win_->helpActionFontInfo);
	AddQAction(idToAction_, cmds_, win_->editActionConvertTabsToSpaces);
	AddQAction(idToAction_, cmds_, win_->editActionToggleBookmark);
	AddQAction(idToAction_, cmds_, win_->editActionNextBookmark);
	AddQAction(idToAction_, cmds_, win_->editActionPrevBookmark);
	AddQAction(idToAction_, cmds_, win_->viewActionHideEditorToolBar);
	AddQAction(idToAction_, cmds_, win_->helpActionCheatSheet);
	AddQAction(idToAction_, cmds_, win_->windowActionHideCustomizer);
	AddQAction(idToAction_, cmds_, win_->viewActionHide3DViewToolBar);
	AddQAction(idToAction_, cmds_, win_->windowActionHideErrorLog);
	AddQAction(idToAction_, cmds_, win_->windowActionSelectEditor);
	AddQAction(idToAction_, cmds_, win_->windowActionSelectConsole);
	AddQAction(idToAction_, cmds_, win_->windowActionSelectCustomizer);
	AddQAction(idToAction_, cmds_, win_->windowActionSelectErrorLog);
	AddQAction(idToAction_, cmds_, win_->windowActionNextWindow);
	AddQAction(idToAction_, cmds_, win_->windowActionPreviousWindow);
	AddQAction(idToAction_, cmds_, win_->editActionInsertTemplate);
	AddQAction(idToAction_, cmds_, win_->helpActionManual);

	using TDx::SpaceMouse::CCommandSet;
	CCommandSet menuBar("Default", "OpenSCAD");
	std::vector<QMenu *> menus = {win_->menu_File,  win_->menuOpenRecent, win_->menuExamples,
																win_->menuExport, win_->menu_Edit,      win_->menu_Design,
																win_->menu_View,  win_->menuHelp,       win_->menuWindow};
	for (auto qmenu : menus) {
		auto qmenu_str = qmenu->title().toStdString();
		qmenu_str.erase(std::remove(qmenu_str.begin(), qmenu_str.end(), '&'), qmenu_str.end());
		TDx::SpaceMouse::CCategory menu(qmenu_str, qmenu_str);
		for (auto a : qmenu->actions()) {
			auto cmd = ptrToStr(a);
			if (idToAction_.find(cmd) != idToAction_.end()) {
				menu.push_back(idToAction_[cmd]->MakeCommand());
			}
		}
		menuBar.push_back(std::move(menu));
	}
	nav->AddCommandSet(menuBar);
	nav->PutActiveCommands(menuBar.GetId());

	std::vector<TDx::CImage> images;
	for (auto &it : idToAction_) {
		if (it.second->HasImage()) {
			images.push_back(it.second->GetCImage());
		}
	}
	nav->AddImages(images);
}

void QActionsHandler::SetActiveCmd(std::string const &cmd)
{
	if (idToAction_.find(cmd) != idToAction_.end()) {
		idToAction_[cmd]->Run();
	}
}
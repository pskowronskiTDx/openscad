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
#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include "src/input/3DMouseInput.h"

class MainWindow;
class QAction;

class QActionCommand
{
public:
	explicit QActionCommand(QAction *, std::string const &rpath);

	TDx::CImage GetCImage() const;
	bool HasImage() const;
	std::string Text() const;
	std::string ToolTips() const;
	std::string Description() const;
	TDx::SpaceMouse::CCommand MakeCommand() const;
	void Run();
	const QAction *Action() const { return qaction_; }

private:
	QAction *qaction_;
	std::string rpath_;
};

struct QActionsHandler {
	MainWindow *win_;
	std::vector<std::shared_ptr<QActionCommand>> cmds_;
	std::unordered_map<std::string, std::shared_ptr<QActionCommand>> idToAction_;
	void ExportApplicationCmds(TDx::SpaceMouse::Navigation3D::CNavigation3D *nav);
	void SetActiveCmd(std::string const &);
};

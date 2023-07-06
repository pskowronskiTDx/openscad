#ifdef ENABLE_3DCONNEXION_NAVLIB
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

#include <QObject>
#include <unordered_map>
#include <Eigen/Dense>
#include "linalg.h"
#include "Camera.h"
#include "renderer.h"
#ifdef __MINGW32__
#include "windows.h"
// same symbol..
#undef DIFFERENCE //#defined in winuser.h
#endif
#include <SpaceMouse/CNavigation3D.hpp>

using CNav3D = TDx::SpaceMouse::Navigation3D::CNavigation3D;
using TDxCommand = TDx::SpaceMouse::CCommand;
using TDxImage = TDx::CImage;
using TDxCategory = TDx::SpaceMouse::CCategory;

constexpr uint32_t sampleCount = 30;

class QGLView;
class QAction;
class MainWindow;

class TDMouseInput : public CNav3D, private QObject
{
public:
	TDMouseInput(MainWindow *p_parent_window, bool multi_threaded = false, bool row_major = false);
	~TDMouseInput();
	bool enableNavigation();
	void disableNavigation();
	void exportCommands();

private:

	class Command {	
	public:	
		Command() = default;
		Command(QAction *const p_qaction, const std::string &icon_file_name);
		TDxCommand toCCommand() const;
		TDxImage getCImage() const; 
		void run();
	private:
		QAction *m_p_qaction{nullptr};
		const std::string m_icon_path{""};
	};

	long GetCoordinateSystem(navlib::matrix_t &) const override;
	long GetCameraMatrix(navlib::matrix_t &) const override;
	long GetIsViewPerspective(navlib::bool_t &) const override;
	long GetViewFOV(double &) const override;
	long GetModelExtents(navlib::box_t &) const override;
	long GetViewExtents(navlib::box_t &) const override;
    long GetViewFrustum(navlib::frustum_t &) const override;
	long GetFrontView(navlib::matrix_t &)const override;
	long GetIsSelectionEmpty(navlib::bool_t &) const;
	long GetPivotPosition(navlib::point_t &) const override;
	long GetHitLookAt(navlib::point_t &) const override;
	long GetPivotVisible(navlib::bool_t &) const override;
	long GetSelectionTransform(navlib::matrix_t &) const override;
	long GetSelectionExtents(navlib::box_t &) const override;
	long GetPointerPosition(navlib::point_t &) const override;
	long GetUnitsToMeters(double&) const override;

	long SetCameraMatrix(const navlib::matrix_t &) override;
	long SetViewFOV(double) override;
	long SetViewExtents(const navlib::box_t &) override;
	long SetViewFrustum(const navlib::frustum_t &) override;
	long SetPivotPosition(const navlib::point_t &) override;
	long SetHitAperture(double);
	long SetHitDirection(const navlib::vector_t &) override;
	long SetHitSelectionOnly(bool) override;
	long SetHitLookFrom(const navlib::point_t &) override;
	long SetPivotVisible(bool) override;
	long SetSelectionTransform(const navlib::matrix_t &) override;
	long SetActiveCommand(std::string) override;
	long IsUserPivot(navlib::bool_t &) const override;

	void registerCommand(QAction *p_qaction, const std::string &icon_file_name);
	void initializeCommandsMap();
	void initializeSampling();
	bool checkQGLView() const;
	TDxCategory getAnimateCategory();

	MainWindow *m_p_parent_window;
	std::unordered_map<std::string, Command> m_id_to_command;
	Eigen::Vector3d m_hit_direction;
	Eigen::Vector3d m_hit_look_from;
	double m_hit_aperture;
	std::vector<Eigen::Vector2d> m_sampling_pattern;
};
#endif
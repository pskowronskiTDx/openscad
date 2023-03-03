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
#include <condition_variable>
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
#include "degree_trig.h"

static inline double rad2deg(double x)
{
	return x * M_RAD2DEG;
}

static inline double deg2rad(double x)
{
	return x * M_DEG2RAD;
}
 
class QGLView;

class TDMouseInput : public TDx::SpaceMouse::Navigation3D::CNavigation3D
{
private:
	bool m_exit_ = false;
	std::shared_ptr<std::mutex> m_cv_m_;
	std::shared_ptr<std::condition_variable> m_cv_;
	// to run cmds (avoid coupling with gui)
	std::function<void(std::string)> onActiveCommand_;
	
	Eigen::Vector3d hitDirection_;
	Eigen::Vector3d hitLookFrom_;
	double hitAperture_;
	bool hitSelectionOnly_;
	QGLView *pQGLView;

public:
	TDMouseInput(QGLView *pQGLView_, bool multiThreaded = false, bool rowMajor = false);
	~TDMouseInput();
	void Run();
	bool Open3DxWare();
	void Close3DxWare();

	long GetCoordinateSystem(navlib::matrix_t &matrix) const override;
	long GetCameraMatrix(navlib::matrix_t &) const override;
	long SetCameraMatrix(const navlib::matrix_t &) override;
	long GetIsViewPerspective(navlib::bool_t &) const override;
	long GetViewFOV(double &) const override;
	long SetViewFOV(double) override;
	long GetModelExtents(navlib::box_t &) const override;
	long GetViewExtents(navlib::box_t &) const override;
	long SetViewExtents(const navlib::box_t &) override;
	long GetViewFrustum(navlib::frustum_t &) const override;
	long SetViewFrustum(const navlib::frustum_t &) override;
	long GetFrontView(navlib::matrix_t &)const override;

	long GetIsSelectionEmpty(navlib::bool_t &s) const;
	long GetPivotPosition(navlib::point_t &) const override;
	long IsUserPivot(navlib::bool_t &) const override;
	long SetPivotPosition(const navlib::point_t &) override;
	long GetHitLookAt(navlib::point_t &) const override;
	long SetHitAperture(double);
	long SetHitDirection(const navlib::vector_t &) override;
	long SetHitSelectionOnly(bool) override;
	long SetHitLookFrom(const navlib::point_t &) override;
	long GetPivotVisible(navlib::bool_t &) const override;
	long SetPivotVisible(bool) override;
	long SetSelectionTransform(const navlib::matrix_t &) override
	{
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}
	long GetSelectionTransform(navlib::matrix_t &) const override
	{
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}
	long GetSelectionExtents(navlib::box_t &) const override
	{
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}

	long GetPointerPosition(navlib::point_t &) const override;
	long SetActiveCommand(std::string) override;
	void SetCommandHandler(std::function<void(std::string)> f);
};

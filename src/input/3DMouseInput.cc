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
#include "3DMouseInput.h"
#include "QGLView.h"

#include <algorithm>

constexpr double MIN_ZOOM = 1 ;
constexpr std::size_t MATRIX_SIZE = 16 * sizeof(double);

TDMouseInput::TDMouseInput(QGLView *pQGLView_ , bool multiThreaded, bool rowMajor) 
	: TDx::SpaceMouse::Navigation3D::CNavigation3D(multiThreaded, rowMajor)
	, m_cv_m_(std::make_shared<std::mutex>())
	, m_cv_(std::make_shared<std::condition_variable>())
	, pQGLView(pQGLView_) 
{
  	QString pivotIconPath = QCoreApplication::applicationDirPath();
  	pivotIconPath.append("/resources/icons/3dx_pivot.png");
  	pQGLView->setPivotIcon(pivotIconPath);
}

TDMouseInput::~TDMouseInput()
{
	Close3DxWare();
}

void TDMouseInput::Run()
{
	std::unique_lock<std::mutex> lock(*m_cv_m_);
	while (!m_exit_) {
		m_cv_->wait(lock);
	}
}

bool TDMouseInput::Open3DxWare()
{
	PutProfileHint("OpenSCAD");
	std::error_code ec;
	EnableNavigation(true, ec);
	PutFrameTimingSource(TimingSource::SpaceMouse);

	if (ec)
	{
		return false;
	}
	return true;
}

void TDMouseInput::Close3DxWare()
{
	EnableNavigation(false);
}

long TDMouseInput::GetCoordinateSystem(navlib::matrix_t &matrix) const 
{
	matrix.m00 = 1.0;
	std::memcpy(&matrix.m00, Eigen::Matrix4d::Identity().eval().data(), MATRIX_SIZE);
	return 0;
}

long TDMouseInput::GetCameraMatrix(navlib::matrix_t &affine) const
{
	std::memcpy(&affine.m00,  pQGLView->cam.getAffine().data(), MATRIX_SIZE);
	return 0;
}

long TDMouseInput::SetCameraMatrix(const navlib::matrix_t &affine)
{
	pQGLView->applyAffine(Eigen::Matrix4d(&affine.m00));
	return 0;
}

long TDMouseInput::GetIsViewPerspective(navlib::bool_t &p) const
{
	p = pQGLView->cam.GetProjection() == Camera::ProjectionType::PERSPECTIVE;
	return 0;
}

long TDMouseInput::GetViewFOV(double &fov) const
{
	fov =deg2rad(pQGLView->cam.fovValue());
	return 0;
}
long TDMouseInput::SetViewFOV(double fov)
{
	pQGLView->cam.setVpf(rad2deg(fov));
	return 0;
}

long TDMouseInput::GetModelExtents(navlib::box_t &nav_box) const
{
	BoundingBox box = pQGLView->renderer->getBoundingBox();
	if(box.isEmpty() || box.isNull()){
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}

	std::memcpy(&nav_box.min.x, box.min().data(), 3*sizeof(double));
	std::memcpy(&nav_box.max.x, box.max().data(), 3*sizeof(double));
	return 0;
}

long TDMouseInput::GetViewExtents(navlib::box_t &bounding_box) const
{
	auto &cam = pQGLView->cam;
	double aspectratio = static_cast<double>(cam.pixel_width) / static_cast<double>(cam.pixel_height);
	double dist = cam.zoomValue();
	double half_height = dist * tan_degrees(cam.fov / 2.0);
	double half_width = half_height * aspectratio;

	dist *= 100.0;

	bounding_box = {-half_width, -half_height, -dist, half_width, half_height, dist};

	return 0;
}

long TDMouseInput::SetViewExtents(const navlib::box_t &bounding_box)
{
	auto &cam = pQGLView->cam;
	double half_height = cam.zoomValue() * tan_degrees(cam.fov / 2.0);
	
	cam.scaleDistance(bounding_box.max.y / half_height);
	
	return 0;
}

long TDMouseInput::GetViewFrustum(navlib::frustum_t &f) const
{
	auto &cam = pQGLView->cam;

	if (cam.projection == Camera::ProjectionType::PERSPECTIVE) {
		// view perspective
		const auto frustum = cam.getFrustum();
		f.right = frustum.right;
		f.left = frustum.left;
		f.bottom = frustum.bottom;
		f.top = frustum.top;
		f.nearVal = frustum.nearVal;
		f.farVal = frustum.farVal;

		return 0;
	}

	return navlib::make_result_code(navlib::navlib_errc::invalid_operation);
}

long TDMouseInput::SetViewFrustum(const navlib::frustum_t &f)
{
	return navlib::make_result_code(navlib::navlib_errc::function_not_supported);
}

long TDMouseInput::GetFrontView(navlib::matrix_t & matrix) const{
	std::memcpy(&matrix.m00, Eigen::Matrix4d::Identity().eval().data(), MATRIX_SIZE);
	return 0;
}

long TDMouseInput::GetIsSelectionEmpty(navlib::bool_t &s)const
{
	s=true;
	return 0;
}

/**
 *  CMDs
 */
long TDMouseInput::SetActiveCommand(std::string cmd) {
	onActiveCommand_(cmd);
	return 0;
}

void TDMouseInput::SetCommandHandler(std::function<void(std::string)> f){
	onActiveCommand_=f;
}


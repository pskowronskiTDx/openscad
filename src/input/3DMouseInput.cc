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
#include "3DMouseInput.h"

#include <algorithm>

constexpr double MIN_ZOOM = 1 ;
constexpr std::size_t MATRIX_SIZE = 16 * sizeof(double);

TDMouseInput::TDMouseInput(std::function<Camera*()> camProvider, std::function<BoundingBox()> bbProvider,
std::function<void(Eigen::Matrix4d const&)> applyAffine,
std::function<void(const Renderer::shaderinfo_t *)> drawer, std::function<void(const Renderer::shaderinfo_t *)> prepareDraw,
std::function<Eigen::Vector2d()> mousePosProvider, bool multiThreaded, bool rowMajor)
	: TDx::SpaceMouse::Navigation3D::CNavigation3D(multiThreaded, rowMajor)
	, isOpen_(false)
	, isRowMajor_(rowMajor)
	, motionFlags_(false)
	, m_cv_m_(std::make_shared<std::mutex>())
	, m_cv_(std::make_shared<std::condition_variable>())
	, pivotPosition_({0,0,0})
	, drawerProvider_(drawer)
	, prepareDraw_(prepareDraw)
	, cameraProvider_(camProvider)
	, boundingBoxProvider_(bbProvider)
	, applyAffine_(applyAffine)
	, mousePosProvider_(mousePosProvider)
{
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
	isOpen_ = true;
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
	std::memcpy(&affine.m00,  cameraProvider_()->getAffine().data(), MATRIX_SIZE);
	return 0;
}

long TDMouseInput::SetCameraMatrix(const navlib::matrix_t &affine)
{
	applyAffine_(Eigen::Matrix4d(&affine.m00));
	return 0;
}

long TDMouseInput::GetIsViewPerspective(navlib::bool_t &p) const
{
	p = cameraProvider_()->GetProjection() == Camera::ProjectionType::PERSPECTIVE;
	return 0;
}

long TDMouseInput::GetViewFOV(double &fov) const
{
	fov =deg2rad(cameraProvider_()->fovValue());
	return 0;
}
long TDMouseInput::SetViewFOV(double fov)
{
	cameraProvider_()->setVpf(rad2deg(fov));
	return 0;
}

long TDMouseInput::GetModelExtents(navlib::box_t &nav_box) const
{
	BoundingBox box = boundingBoxProvider_();
	if(box.isEmpty() || box.isNull()){
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}

	std::memcpy(&nav_box.min.x, box.min().data(), 3*sizeof(double));
	std::memcpy(&nav_box.max.x, box.max().data(), 3*sizeof(double));
	return 0;
}

long TDMouseInput::GetViewExtents(navlib::box_t &bounding_box) const
{
	auto & cam = *cameraProvider_();

	double aspectratio = static_cast<double>(cam.pixel_width) / static_cast<double>(cam.pixel_height);

	double dist = cam.zoomValue();

	double half_height = dist * tan_degrees(cam.fov / 2);

	bounding_box = {-half_height * aspectratio, -half_height, -100 * dist,
					 half_height * aspectratio,  half_height,  100 * dist};
	return 0;
}

long TDMouseInput::SetViewExtents(const navlib::box_t &bounding_box)
{
	auto &cam = *cameraProvider_();

	double dist = cam.zoomValue();

	double aspectratio = static_cast<double>(cam.pixel_width) / static_cast<double>(cam.pixel_height);

	double half_height = dist * tan_degrees(cam.fov / 2);

	cam.zoom(dist * half_height / bounding_box.max.y, false);

	return 0;
}

long TDMouseInput::GetViewFrustum(navlib::frustum_t &f) const
{
	auto &cam = *cameraProvider_();

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


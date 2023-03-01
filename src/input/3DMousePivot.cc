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
#include "OpenGlUtils.h"

Eigen::Vector3d TransformVGet(Eigen::Vector3d const &p, Eigen::Matrix4d const& transformation)
{
	return (p.transpose() * transformation.block<3, 3>(0, 0)).eval();
}

Eigen::Vector3d TransformVSet(Eigen::Vector3d const &p, Eigen::Matrix4d const& transformation)
{
	return TransformVGet(p, transformation.inverse());
}


long TDMouseInput::GetPivotPosition(navlib::point_t &p) const
{	
	std::memcpy(&p.x, pivotPosition_.data(), sizeof(double) * pivotPosition_.size());
	return 0;
}

long TDMouseInput::IsUserPivot(navlib::bool_t &p) const
{
	p = false;
	return 0;
}

long TDMouseInput::SetPivotPosition(const navlib::point_t &p)
{
	std::memcpy(pivotPosition_.data(), &p.x, sizeof(double) * pivotPosition_.size());
	cameraProvider_()->SetPivot(pivotPosition_);
	return 0;
}

long TDMouseInput::GetHitLookAt(navlib::point_t &p) const
{
	double diameter = hitAperture_;
	const auto &frustum = cameraProvider_()->getFrustum();
	double distance = GetZBufferDepth(hitLookFrom_, hitDirection_, hitAperture_, *cameraProvider_(),
																		prepareDraw_, drawerProvider_);
	if (distance < frustum.farVal - 0.00005) {
		auto hitlookat = (hitLookFrom_ + hitDirection_ * distance).eval();
		//only for drawing should be removed
		cameraProvider_()->hitLookAt_=hitlookat;
		std::memcpy(&p.x, hitlookat.data(),hitlookat.size()*sizeof(double));
		return 0;
	}
	else {
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	}
}

long TDMouseInput::SetHitAperture(double hitAperture)
{
	hitAperture_ = hitAperture;
	return 0;
}

long TDMouseInput::SetHitDirection(const navlib::vector_t &hitDir)
{
	std::memcpy(hitDirection_.data(), &hitDir.x, sizeof(double) * hitDirection_.size());
	return 0;
}

long TDMouseInput::SetHitSelectionOnly(bool hso)
{
	hitSelectionOnly_ = hso;
	return 0;
}

long TDMouseInput::SetHitLookFrom(const navlib::point_t &hitLookFrom)
{
	std::memcpy(hitLookFrom_.data(), &hitLookFrom.x, sizeof(double) * hitLookFrom_.size());
	//only for drawing should be removed
	cameraProvider_()->hitLookFrom_=hitLookFrom_;
	return 0;
}

long TDMouseInput::GetPivotVisible(navlib::bool_t &v) const
{
	v = cameraProvider_()->GetPivot().first;
	return 0;
}
long TDMouseInput::SetPivotVisible(bool v)
{
	cameraProvider_()->SetPivotVisible(v);
	return 0;
}

// ptr
long TDMouseInput::GetPointerPosition(navlib::point_t & p) const{
	auto mousePos = GetCursorWorldCoordinates(*cameraProvider_(), prepareDraw_, drawerProvider_,mousePosProvider_());
	std::cout << mousePos.transpose() << std::endl;
	std::memcpy(&p.x,mousePos.data(), mousePos.size()*sizeof(double));
	return 0;
}
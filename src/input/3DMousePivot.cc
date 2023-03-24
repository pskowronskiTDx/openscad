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
#include "QGLView.h"

long TDMouseInput::GetPivotPosition(navlib::point_t &p) const
{	
	std::memcpy(&p.x, pQGLView->getPivotPosition().data(), sizeof(double) * 3u);
	return 0;
}

long TDMouseInput::IsUserPivot(navlib::bool_t &p) const
{
	p = false;
	return 0;
}

long TDMouseInput::SetPivotPosition(const navlib::point_t &p)
{
	pQGLView->setPivotPosition({p.x, p.y, p.z});
	return 0;
}

long TDMouseInput::GetHitLookAt(navlib::point_t &p) const
{	
	Eigen::Vector3d hit = getHitPoint(pQGLView, samplingPattern, hitAperture_, hitDirection_, hitLookFrom_);
		
	if(hit.z() == std::numeric_limits<double>::max())
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	
	p.x = hit.x();
	p.y = hit.y();
	p.z = hit.z();

	return 0;

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
	return 0;
}

long TDMouseInput::GetPivotVisible(navlib::bool_t &v) const
{
	v = pQGLView->getPivotVisibility();
	return 0;
}
long TDMouseInput::SetPivotVisible(bool v)
{
	pQGLView->setPivotVisibility(v);
	return 0;
}

// ptr
long TDMouseInput::GetPointerPosition(navlib::point_t & p) const {

	QPoint cursorPosition = QCursor::pos();
	cursorPosition = pQGLView->mapFromGlobal(cursorPosition);

	Eigen::Vector3d cursorCoordinates = getCursorInWorld(pQGLView, cursorPosition.x(), cursorPosition.y());
	std::memcpy(&p.x, cursorCoordinates.data(), cursorCoordinates.size() * sizeof(double));

	return 0;
}
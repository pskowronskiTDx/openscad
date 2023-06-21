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
#include "MainWindow.h"

long TDMouseInput::GetPivotPosition(navlib::point_t &p) const
{	
	std::memcpy(&p.x, m_p_main_window->qglview->getPivotPosition().data(), sizeof(double) * 3u);
	return 0;
}

long TDMouseInput::IsUserPivot(navlib::bool_t &p) const
{
	p = false;
	return 0;
}

long TDMouseInput::SetPivotPosition(const navlib::point_t &p)
{
	m_p_main_window->qglview->setPivotPosition({p.x, p.y, p.z});
	return 0;
}

long TDMouseInput::GetHitLookAt(navlib::point_t &p) const
{	
	QOpenGLContext *oldContext = getGLContext();
	m_p_main_window->qglview->makeCurrent();

	Eigen::Vector3d hit = getHitPoint(m_p_main_window->qglview, m_sampling_pattern, m_hit_aperture, m_hit_direction, m_hit_look_from);
	
	setGLContext(oldContext);

	if(hit.z() == std::numeric_limits<double>::max())
		return navlib::make_result_code(navlib::navlib_errc::no_data_available);
	
	p.x = hit.x();
	p.y = hit.y();
	p.z = hit.z();

	return 0;

}

long TDMouseInput::SetHitAperture(double hitAperture)
{
	m_hit_aperture = hitAperture;
	return 0;
}

long TDMouseInput::SetHitDirection(const navlib::vector_t &hitDir)
{
	std::memcpy(m_hit_direction.data(), &hitDir.x, sizeof(double) * m_hit_direction.size());
	return 0;
}

long TDMouseInput::SetHitSelectionOnly(bool hso)
{
	return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long TDMouseInput::SetHitLookFrom(const navlib::point_t &hitLookFrom)
{
	std::memcpy(m_hit_look_from.data(), &hitLookFrom.x, sizeof(double) * m_hit_look_from.size());
	return 0;
}

long TDMouseInput::GetPivotVisible(navlib::bool_t &v) const
{
	v = m_p_main_window->qglview->getPivotVisibility();
	return 0;
}
long TDMouseInput::SetPivotVisible(bool v)
{
	m_p_main_window->qglview->setPivotVisibility(v);
	m_p_main_window->qglview->repaint();
	return 0;
}

// ptr
long TDMouseInput::GetPointerPosition(navlib::point_t & p) const {

	QPoint cursorPosition = QCursor::pos();

	QOpenGLContext *oldContext = getGLContext();
	m_p_main_window->qglview->makeCurrent();

	cursorPosition = m_p_main_window->qglview->mapFromGlobal(cursorPosition);
	Eigen::Vector3d cursorCoordinates = getCursorInWorld(m_p_main_window->qglview, cursorPosition.x(), cursorPosition.y());

	setGLContext(oldContext);

	std::memcpy(&p.x, cursorCoordinates.data(), cursorCoordinates.size() * sizeof(double));


	return 0;
}
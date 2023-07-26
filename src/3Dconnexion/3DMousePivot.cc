// -------------------------------------------------------------------------------------------------
// This source file is part of the OpenSCAD project.
//
// Copyright (c) 2014-2023 3Dconnexion.
//
// This source code is released under the GNU General Public License, (see "LICENSE").
// -------------------------------------------------------------------------------------------------

#include "3DMouseInput.h"
#include "MainWindow.h"
#include "OpenGLUtils.h"

long TDMouseInput::GetPivotPosition(navlib::point_t &p) const
{
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  const auto pivotPosition = m_p_parent_window->qglview->getPivotPosition();

  std::copy_n(pivotPosition.data(), pivotPosition.size(), &p.x);

  return 0;
}

long TDMouseInput::IsUserPivot(navlib::bool_t &p) const
{
  p = false;
  return 0;
}

long TDMouseInput::SetPivotPosition(const navlib::point_t &p)
{
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  m_p_parent_window->qglview->setPivotPosition({p.x, p.y, p.z});
  return 0;
}

long TDMouseInput::GetHitLookAt(navlib::point_t &p) const
{
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  QOpenGLContext *oldContext = getGLContext();
  m_p_parent_window->qglview->makeCurrent();

  Eigen::Vector3d hit = getHitPoint(m_p_parent_window->qglview, m_sampling_pattern, m_hit_aperture, m_hit_direction, m_hit_look_from);

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
  std::copy_n(&hitDir.x, m_hit_direction.size(), m_hit_direction.data());
  return 0;
}

long TDMouseInput::SetHitSelectionOnly(bool hso)
{
  return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long TDMouseInput::SetHitLookFrom(const navlib::point_t &hitLookFrom)
{
  std::copy_n(&hitLookFrom.x, m_hit_look_from.size(), m_hit_look_from.data() );
  return 0;
}

long TDMouseInput::GetPivotVisible(navlib::bool_t &v) const
{
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  v = m_p_parent_window->qglview->getPivotVisibility();
  return 0;
}
long TDMouseInput::SetPivotVisible(bool v)
{
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  m_p_parent_window->qglview->setPivotVisibility(v);
  m_p_parent_window->qglview->repaint();
  return 0;
}

long TDMouseInput::GetPointerPosition(navlib::point_t & p) const {
  if(!checkQGLView()) {
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
  }

  QPoint cursorPosition = QCursor::pos();

  QOpenGLContext *oldContext = getGLContext();
  m_p_parent_window->qglview->makeCurrent();

  cursorPosition = m_p_parent_window->qglview->mapFromGlobal(cursorPosition);
  Eigen::Vector3d cursorCoordinates = getCursorInWorld(m_p_parent_window->qglview,
                                                       cursorPosition.x(),
                                                       cursorPosition.y());

  setGLContext(oldContext);

  std::copy_n(cursorCoordinates.data(), cursorCoordinates.size(), &p.x);

  return 0;
}
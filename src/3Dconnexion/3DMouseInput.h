// -------------------------------------------------------------------------------------------------
// This source file is part of the OpenSCAD project.
//
// Copyright (c) 2014-2023 3Dconnexion.
//
// This source code is released under the GNU General Public License, (see "LICENSE").
// -------------------------------------------------------------------------------------------------

#ifdef ENABLE_3DCONNEXION_NAVLIB
#ifndef TDMOUSE_INPUT_H
#define TDMOUSE_INPUT_H

#include <QObject>
#include <unordered_map>
#include <string>
#include <Eigen/Dense>
#include <SpaceMouse/CNavigation3D.hpp>

using CNav3D = TDx::SpaceMouse::Navigation3D::CNavigation3D;
using TDxCommand = TDx::SpaceMouse::CCommand;
using TDxImage = TDx::CImage;
using TDxCategory = TDx::SpaceMouse::CCategory;

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
  class Command
  {
  public:
    Command() = default;
    Command(QAction *const p_qaction);
    TDxCommand toCCommand() const;
    TDxImage getCImage() const;
    void run();

   private:
    QAction *m_p_qaction{nullptr};
  };

  long GetCoordinateSystem(navlib::matrix_t &) const override;
  long GetCameraMatrix(navlib::matrix_t &) const override;
  long GetIsViewPerspective(navlib::bool_t &) const override;
  long GetViewFOV(double &) const override;
  long GetModelExtents(navlib::box_t &) const override;
  long GetViewExtents(navlib::box_t &) const override;
  long GetViewFrustum(navlib::frustum_t &) const override;
  long GetFrontView(navlib::matrix_t &)const override;
  long GetIsSelectionEmpty(navlib::bool_t &) const override;
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
  long SetHitAperture(double) override;
  long SetHitDirection(const navlib::vector_t &) override;
  long SetHitSelectionOnly(bool) override;
  long SetHitLookFrom(const navlib::point_t &) override;
  long SetPivotVisible(bool) override;
  long SetSelectionTransform(const navlib::matrix_t &) override;
  long SetActiveCommand(std::string) override;
  long IsUserPivot(navlib::bool_t &) const override;

  void registerCommand(QAction *p_qaction);
  void initializeCommandsMap();
  void initializeSampling();
  bool checkQGLView() const;
  TDxCategory getAnimateCategory() const;

  MainWindow *m_p_parent_window;
  std::unordered_map<std::string, Command> m_id_to_command;
  Eigen::Vector3d m_hit_direction;
  Eigen::Vector3d m_hit_look_from;
  double m_hit_aperture;
  std::vector<Eigen::Vector2d> m_sampling_pattern;
};
#endif // TDMOUSE_INPUT_H
#endif // ENABLE_3DCONNEXION_NAVLIB
// -------------------------------------------------------------------------------------------------
// This source file is part of the OpenSCAD project.
//
// Copyright (c) 2014-2023 3Dconnexion.
//
// This source code is released under the GNU General Public License, (see "LICENSE").
// -------------------------------------------------------------------------------------------------

#ifndef OPENGL_UTILS_H
#define OPENGL_UTILS_H

#include <Eigen/Dense>

class QGLView;

Eigen::Vector3d getHitPoint(QGLView *const pQGLView,
                            const std::vector<Eigen::Vector2d> &samplingPattern,
                            const double &apertureInWorld,
                            const Eigen::Vector3d &lookDirection,
                            const Eigen::Vector3d &lookFrom);

Eigen::Vector3d getCursorInWorld(const QGLView *const pQGLView, uint32_t cursorX, uint32_t cursorY);

#endif
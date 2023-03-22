#pragma once

#include <Eigen/Dense>

class QGLView;

Eigen::Vector3d getHitPoint(QGLView *const pQGLView,
						    const std::vector<Eigen::Vector2d> &samplingPattern,
							const double &apertureInWorld,
							const Eigen::Vector3d &lookDirection,
							const Eigen::Vector3d &lookFrom);

Eigen::Vector3d getCursorInWorld(const QGLView *const pQGLView, uint32_t cursorX, uint32_t cursorY);

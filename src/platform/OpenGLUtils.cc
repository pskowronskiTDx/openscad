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

#include <memory>
#include <Eigen/Dense>

#include "degree_trig.h"
#include "OpenGlUtils.h"
#include "system-gl.h"
#include "QGLView.h"
#include "Camera.h"
#include "renderer.h"

#include <QOpenGLFramebufferObject>

double projectAperture(const Camera &camera, const double &aperture)
{
  Camera::Frustum frustum = camera.getFrustum();
  double result = (aperture * static_cast<double>(camera.pixel_width)) / (frustum.right - frustum.left);

  if (camera.getProjection() == Camera::ProjectionType::PERSPECTIVE) {
    const double nf = 0.01 / frustum.nearVal;
    result /= nf;
  }
  return result;
}

void glModelView(Camera const &camera, const Eigen::Vector3d &look_direction, const Eigen::Vector3d &look_from)
{
  const Eigen::Affine3d affine = camera.getAffine();
  const Eigen::Vector3d look_at = look_direction + look_from;
  // The camera's up axis is its y-axis.
  const Eigen::Vector3d up(affine(0, 1), affine(1, 1), affine(2, 1));

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(look_from[0], look_from[1], look_from[2],
            look_at[0], look_at[1], look_at[2], // center
            up[0], up[1], up[2]);				// up
}

void glPickInit(Camera const &cam, const double &aperture)
{
  glClearColor(0, 0, 0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glDisable(GL_LIGHTING);

  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  const auto frustum = cam.getFrustum();

  switch (cam.projection) {
  case Camera::ProjectionType::PERSPECTIVE: {
    const double aspectratio = (frustum.right - frustum.left) / (frustum.top - frustum.bottom);
    gluPerspective(cam.fov, aspectratio, frustum.nearVal, frustum.farVal);
    break;
  }
  case Camera::ProjectionType::ORTHOGONAL: {
    glOrtho(frustum.left, frustum.right, frustum.bottom, frustum.top, 0, frustum.farVal - frustum.nearVal);
    break;
  }
  default:
    break;
  }
}

Eigen::Vector3d getHitPoint(QGLView *const p_qglview,
                            const std::vector<Eigen::Vector2d> &sampling_pattern,
                            const double &aperture_in_world,
                            const Eigen::Vector3d &look_direction,
                            const Eigen::Vector3d &look_from)
{
  if (p_qglview == nullptr) {
    return Vector3d();
  }

  if (p_qglview->renderer == nullptr) {
    return Vector3d();
  }

  p_qglview->renderer->prepare(true, false);

  QOpenGLFramebufferObjectFormat fbo_format;
  fbo_format.setSamples(0);
  fbo_format.setAttachment(QOpenGLFramebufferObject::Depth);

  const GLint viewport[4] = {
    0,
    0,
    static_cast<GLint>(p_qglview->cam.pixel_width),
    static_cast<GLint>(p_qglview->cam.pixel_height)};

  glViewport(0, 0, viewport[2], viewport[3]);

  std::unique_ptr<QOpenGLFramebufferObject> framebuffer;
  framebuffer.reset(new QOpenGLFramebufferObject(viewport[2], viewport[3], fbo_format));
  framebuffer->release();
  framebuffer->bind();

  const double aperture = projectAperture(p_qglview->cam, aperture_in_world);

  glPickInit(p_qglview->cam, aperture);
  glModelView(p_qglview->cam, look_direction, look_from);

  GLdouble model_matrix[16];
  glGetDoublev(GL_MODELVIEW_MATRIX, model_matrix);

  GLdouble projection_matrix[16];
  glGetDoublev(GL_PROJECTION_MATRIX, projection_matrix);

  p_qglview->renderer->draw(true, false);

  glFlush();
  glFinish();

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glPopAttrib();
  glFinish();

  GLdouble x, y, z = 1.0;
  const double screen_center_x = static_cast<double>(viewport[2] / 2);
  const double screen_center_y = static_cast<double>(viewport[3] / 2);

  for (auto &sample : sampling_pattern) {
    double sample_px_x = screen_center_x + sample.x() * aperture;
    double sample_px_y = screen_center_y - sample.y() * aperture;
    float sample_px_z;

    glReadPixels(sample_px_x, sample_px_y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &sample_px_z);

    if ((sample_px_z < z) && (sample_px_z > 0.0001)) {
      z = sample_px_z;
      x = sample_px_x;
      y = sample_px_y;
      }
	}

    framebuffer->release();

    if (z == 1.0) {
      return Eigen::Vector3d(z, z, std::numeric_limits<double>::max());
    }

    gluUnProject(x, y, z, model_matrix, projection_matrix, viewport, &x, &y, &z);

    return Eigen::Vector3d(x, y, z);
}

Eigen::Vector3d getCursorInWorld(const QGLView *const p_qglview, uint32_t cursor_x, uint32_t cursor_y)
{
    glViewport(0, 0, p_qglview->cam.pixel_width, p_qglview->cam.pixel_height);
    p_qglview->setupCamera();
    glTranslated(p_qglview->cam.object_trans.x(),
                 p_qglview->cam.object_trans.y(),
                 p_qglview->cam.object_trans.z());

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    GLdouble projection_matrix[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projection_matrix);

    GLdouble model_view[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, model_view);

    GLdouble x, y, z = 0.0;

    // x/y is originated topleft, so turn y around
    gluUnProject(cursor_x, p_qglview->cam.pixel_height - cursor_y, 0.0,
                 model_view, projection_matrix, viewport, &x, &y, &z);

    return Eigen::Vector3d(x, y, z);
}
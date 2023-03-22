#pragma once

#include <array>
#include <Eigen/Dense>
#include "Camera.h"
#include "QGLView.h"
#include "renderer.h"

Renderer::shaderinfo_t CreateShaderInfo(const std::string& shaderVertexfile, const std::string& shaderFragfile);
/**
 * position
 * direction
 * diameter
 * frustrum
 * */
double GetZBufferDepth(Eigen::Vector3d const &position, Eigen::Vector3d const &direction,
											 double diameter, Camera const& cam, std::function<void(const Renderer::shaderinfo_t *)> prepareDrawer,
											 std::function<void(const Renderer::shaderinfo_t *)> drawer);

Eigen::Vector3d GetCursorWorldCoordinates(QGLView *view, Eigen::Vector2d const& mousePos);
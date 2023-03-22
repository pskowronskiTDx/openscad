#include <memory>
#include <Eigen/Dense>

#include "degree_trig.h"
#include "OpenGlUtils.h"
#include "system-gl.h"
#include "QGLView.h"
#include "Camera.h"
#include "renderer.h"

#include <QOpenGLFramebufferObject>

#if DEBUG
#pragma GCC optimize ("O0")
#pragma message "Compiling file with optimize -O0"
#endif

// Helper functions (visibility limited to the translation unit)

uint32_t projectAperture(const Camera &camera, const double &aperture) {
	const double shortestSidePx = static_cast<double>(std::min(camera.pixel_height, camera.pixel_width));
	const double frustumHeight = camera.getFrustum().top - camera.getFrustum().bottom;
	const double frustumWidth = camera.getFrustum().right - camera.getFrustum().left;
	const double shortestSideWorld = std::min(frustumHeight, frustumWidth);
	return static_cast<uint32_t>((shortestSidePx * aperture) / shortestSideWorld);
}

void glModelView(Camera const &camera, const Eigen::Vector3d &lookdir, const Eigen::Vector3d &lookfrom)
{
	const Eigen::Affine3d affine = camera.getAffine();
	const Eigen::Vector3d lookAt = lookdir + lookfrom;
	// The camera's up axis is its y-axis.
	const Eigen::Vector3d up(affine(0,1), affine(1,1), affine(2,1));

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	gluLookAt(lookfrom[0], lookfrom[1], lookfrom[2],
			  lookAt[0], lookAt[1], lookAt[2],		// center
			  up[0], up[1], up[2]);                 // up
}

void glPickInit(Camera const &cam, const double &aperture)
{
	glClearColor(0, 0, 0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	gluPickMatrix(viewport[2] / 2.0, viewport[3]/ 2.0, aperture, aperture, viewport);

	const double dist = cam.zoomValue();
	const double aspectratio = static_cast<double>(viewport[2]) / static_cast<double>(viewport[3]);

	switch (cam.projection) {
	case Camera::ProjectionType::PERSPECTIVE: {
		gluPerspective(cam.fov, aspectratio, 0.1 * dist, 100 * dist);
		break;
	} 
	case Camera::ProjectionType::ORTHOGONAL: {
		const double height = dist * tan_degrees(cam.fov / 2.0);
		glOrtho(-height * aspectratio, height * aspectratio, -height, height, -100.0 * dist, 100.0 * dist);
		break;
	}
	default:
	break;
	}
}

Renderer::shaderinfo_t createShaderInfo(const std::string &shaderVertexfile, const std::string &shaderFragfile)
{
	Renderer::shaderinfo_t shaderinfo;
	std::string vs_str = Renderer::loadShaderSource(shaderVertexfile);
	std::string fs_str = Renderer::loadShaderSource(shaderFragfile);
	const char *vs_source = vs_str.c_str();
	const char *fs_source = fs_str.c_str();

	int shaderstatus;
	// Compile the shaders
	auto vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, (const GLchar **)&vs_source, nullptr);
	glCompileShader(vs);
	glGetShaderiv(vs, GL_COMPILE_STATUS, &shaderstatus);
	if (shaderstatus != GL_TRUE) {
		int loglen;
		char logbuffer[1000];
		glGetShaderInfoLog(vs, sizeof(logbuffer), &loglen, logbuffer);
		fprintf(stderr, __FILE__ ": OpenGL vertex shader Error:\n%.*s\n\n", loglen, logbuffer);
	}

	auto fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, (const GLchar **)&fs_source, nullptr);
	glCompileShader(fs);
	glGetShaderiv(fs, GL_COMPILE_STATUS, &shaderstatus);
	if (shaderstatus != GL_TRUE) {
		int loglen;
		char logbuffer[1000];
		glGetShaderInfoLog(fs, sizeof(logbuffer), &loglen, logbuffer);
		fprintf(stderr, __FILE__ ": OpenGL fragment shader Error:\n%.*s\n\n", loglen, logbuffer);
	}

	// Link
	auto selecthader_prog = glCreateProgram();
	glAttachShader(selecthader_prog, vs);
	glAttachShader(selecthader_prog, fs);
	glLinkProgram(selecthader_prog);

	GLint status;
	glGetProgramiv(selecthader_prog, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		int loglen;
		char logbuffer[1000];
		glGetProgramInfoLog(selecthader_prog, sizeof(logbuffer), &loglen, logbuffer);
		fprintf(stderr, __FILE__ ": OpenGL Program Linker Error:\n%.*s\n\n", loglen, logbuffer);
	}
	else {
		int loglen;
		char logbuffer[1000];
		glGetProgramInfoLog(selecthader_prog, sizeof(logbuffer), &loglen, logbuffer);
		if (loglen > 0) {
			fprintf(stderr, __FILE__ ": OpenGL Program Link OK:\n%.*s\n\n", loglen, logbuffer);
		}
		glValidateProgram(selecthader_prog);
		glGetProgramInfoLog(selecthader_prog, sizeof(logbuffer), &loglen, logbuffer);
		if (loglen > 0) {
			fprintf(stderr, __FILE__ ": OpenGL Program Validation results:\n%.*s\n\n", loglen, logbuffer);
		}
	}

	shaderinfo.progid = selecthader_prog;
	shaderinfo.type = Renderer::SELECT_RENDERING;

	GLint identifier = glGetUniformLocation(selecthader_prog, "frag_idcolor");
	if (identifier < 0) {
		fprintf(stderr, __FILE__ ": OpenGL symbol retrieval went wrong, id is %i\n\n", identifier);
		shaderinfo.data.select_rendering.identifier = 0;
	}
	else {
		shaderinfo.data.select_rendering.identifier = identifier;
	}

	return shaderinfo;
}

// Public funtions (visible if OpenGlUtils.h included)

Eigen::Vector3d getHitPoint(QGLView *const pQGLView, 
							const std::vector<Eigen::Vector2d> &samplingPattern, 
							const double &apertureInWorld,
							const Eigen::Vector3d &lookDirection,
							const Eigen::Vector3d &lookFrom)
{
	const Renderer::shaderinfo_t shaderInfo = createShaderInfo("MouseSelector.vert", "MouseSelector.frag");

	pQGLView->renderer->prepare(true, false, &shaderInfo);

	QOpenGLFramebufferObjectFormat fboFormat;
	fboFormat.setSamples(0);	
	fboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
	
	const GLint viewport[4] = {
		0,
		0, 
		static_cast<GLint>(pQGLView->cam.pixel_width),
		static_cast<GLint>(pQGLView->cam.pixel_height)};

	glViewport(0, 0, viewport[2], viewport[3]);

	std::unique_ptr<QOpenGLFramebufferObject> framebuffer;
	framebuffer.reset(new QOpenGLFramebufferObject(viewport[2], viewport[3], fboFormat));
	framebuffer->release();
	framebuffer->bind();
                   
	const double aperture = projectAperture(pQGLView->cam, apertureInWorld);

	glPickInit(pQGLView->cam, aperture);
	glModelView(pQGLView->cam, lookDirection, lookFrom);

	GLdouble modelMatrix[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	GLdouble projectionMatrix[16];
	glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);
	
	pQGLView->renderer->draw(true, false, &shaderInfo);

	glFlush();
	glFinish();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glPopAttrib();
	glFinish();

	GLdouble x, y, z = 1.0;
	const double screenCenterX = static_cast<double>(viewport[2] / 2);
	const double screenCenterY = static_cast<double>(viewport[3] / 2);

	for(auto &sample : samplingPattern) {

		double sampleX = screenCenterX + sample.x() * aperture;
		double sampleY = screenCenterY - sample.y() * aperture;
		float sampleZ;

		glReadPixels(sampleX, sampleY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &sampleZ);

		if((sampleZ < z) && (sampleZ > 0.0001)) {
			z = sampleZ;
			x = sampleX;
			y = sampleY;
		}
	}

	framebuffer->release();

	if(z == 1.0) {
		return Eigen::Vector3d(z, z, std::numeric_limits<double>::max());
	}
		
	gluUnProject(x, y, z, modelMatrix, projectionMatrix, viewport, &x, &y, &z);

	return Eigen::Vector3d(x, y, z);
}

Eigen::Vector3d getCursorInWorld(const QGLView *const pQGLView, uint32_t cursorX, uint32_t cursorY) {

	const Camera &cam = pQGLView->cam;
	const GLint viewport[4] = {
		0,
		0,
		static_cast<GLint>(cam.pixel_width),
		static_cast<GLint>(cam.pixel_height)};
	
	GLdouble projectionMatrix[16];
	pQGLView->getCurrentProjection(projectionMatrix);

  	GLdouble x = 0.0, y = 0.0, z = 0.0;

	gluUnProject(cursorX, viewport[3] - cursorY, 0.05,
				 cam.getAffine().inverse().data(), projectionMatrix, viewport,
				 &x, &y, &z);

	return Eigen::Vector3d(x, y, z);
}


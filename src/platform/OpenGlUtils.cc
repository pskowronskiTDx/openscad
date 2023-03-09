#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <Eigen/Dense>

#include "degree_trig.h"
#include "OpenGlUtils.h"
#include "system-gl.h"
#include "renderer.h"
#include <QOpenGLFramebufferObject>
#include <QCursor>
#include <QImage>

#include "iostream"


constexpr double kEpsilon5 = 1.0e-5;

#if DEBUG
#pragma GCC optimize ("O0")
#pragma message "Compiling file with optimize -O0"

#define DumpHitBuffer 1
#endif

#if DumpHitBuffer
#pragma pack(push, 1)
typedef struct tagBITMAPINFOHEADER {
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
} BITMAPINFOHEADER, FAR *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
	BYTE rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[1];
} BITMAPINFO, FAR *LPBITMAPINFO, *PBITMAPINFO;

typedef struct tagBITMAPFILEHEADER {
	WORD bfType;
	DWORD bfSize;
	WORD bfReserved1;
	WORD bfReserved2;
	DWORD bfOffBits;
} BITMAPFILEHEADER, FAR *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;
#pragma pack(pop)

void SaveAsBitmap(LPCWSTR lpFileName, GLsizei width, GLsizei height, std::vector<RGBQUAD> const & bits) {
  BITMAPINFO bmi = {0};

  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = height;
  bmi.bmiHeader.biPlanes = 1; // we are assuming that there is only one plane
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biSizeImage = static_cast<DWORD>(bits.size());

  // no compression this is an rgb bitmap
  bmi.bmiHeader.biCompression = 0; // BI_RGB;


  // all device colours are important
  bmi.bmiHeader.biClrImportant = 0;

  HANDLE hFile = CreateFile(lpFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, NULL);

  if (hFile != INVALID_HANDLE_VALUE) {
    DWORD dwTemp;
    BITMAPFILEHEADER hdr = {0};
    // type == BM
    hdr.bfType = 0x4d42;

    hdr.bfSize = (sizeof(BITMAPFILEHEADER) + bmi.bmiHeader.biSize +
                  bmi.bmiHeader.biClrUsed * sizeof(RGBQUAD) + bmi.bmiHeader.biSizeImage);
    hdr.bfReserved1 = 0;
    hdr.bfReserved2 = 0;

    hdr.bfOffBits =
        sizeof(BITMAPFILEHEADER) + bmi.bmiHeader.biSize + bmi.bmiHeader.biClrUsed * sizeof(RGBQUAD);

    // write the bitmap file header to file
    WriteFile(hFile, (LPVOID)&hdr, sizeof(BITMAPFILEHEADER), &dwTemp, NULL);

    // write the bitmap header to file
    WriteFile(hFile, (LPVOID)&bmi.bmiHeader,
              sizeof(BITMAPINFOHEADER) + bmi.bmiHeader.biClrUsed * sizeof(RGBQUAD), &dwTemp, NULL);

    // copy the bitmap colour data into the file
    WriteFile(hFile, (LPSTR)&bits[0], bmi.bmiHeader.biSizeImage, &dwTemp, NULL);

    CloseHandle(hFile);
  }
}
#endif


void GlModelView(Camera const &cam, Eigen::Vector3d const& lookdir, Eigen::Vector3d const& lookfrom)
{
	Eigen::Affine3d affine = cam.getAffine();
	Eigen::Vector3d lookAt = lookdir + lookfrom;
	// The camera's up axis is its y-axis.
	Eigen::Vector3d up(affine(0,1), affine(1,1), affine(2,1));

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	gluLookAt(lookfrom[0], lookfrom[1], lookfrom[2],
			  lookAt[0], lookAt[1], lookAt[2],		// center
			  up[0], up[1], up[2]);                 // up
}

/**
 * lookfrom
 * direction
 * diameter
 * frustum
 *
 * */
Eigen::Vector2d ComputeAperture(GLint viewport[4], double diameter, Camera::Frustum const &f)
{
	int aperturex = 0;
	int aperturey = 0;
	if (viewport[2] > viewport[3]) {
		aperturex =
				1 + static_cast<int>(diameter * static_cast<double>(viewport[2]) / (f.right - f.left));
		if (aperturex > viewport[3]) {
			aperturey = viewport[3];
		}
		aperturey = aperturex;
	}
	else {
		aperturey =
				1 + static_cast<int>(diameter * static_cast<double>(viewport[3]) / (f.top - f.bottom));
		if (aperturey > viewport[2]) {
			aperturex = viewport[2];
		}
		aperturex = aperturey;
	}

	// Ensure x and y are odd so that we know where the central pixel is
	if (!(aperturex & 0x01)) {
		--aperturex;
	}
	if (!(aperturey & 0x01)) {
		--aperturey;
	}
	return {aperturex, aperturey};
}


Renderer::shaderinfo_t CreateShaderInfo(const std::string &shaderVertexfile, const std::string &shaderFragfile)
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

void GlPickInit(Camera const& cam, Eigen::Vector2d const& center, Eigen::Vector2d const& region)
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
	gluPickMatrix(center.x(), center.y(), region.x(), region.y(), viewport);

	double dist = cam.zoomValue();
	double aspectratio = static_cast<double>(viewport[2]) / static_cast<double>(viewport[3]);

	switch (cam.projection) {
	case Camera::ProjectionType::PERSPECTIVE: {
		gluPerspective(cam.fov, aspectratio, 0.1 * dist, 100 * dist);
		break;
	} 
	default:
	case Camera::ProjectionType::ORTHOGONAL: {
		auto height = dist * tan_degrees(cam.fov / 2.0);
		glOrtho(-height * aspectratio, height * aspectratio, -height, height, -100.0 * dist, 100.0 * dist);
		break;
	}
	}
}


Eigen::Vector3d GetHitPoint( Camera const &cam, std::function<void(const Renderer::shaderinfo_t *)> prepareDrawer,
											 std::function<void(const Renderer::shaderinfo_t *)> drawer) {
	Eigen::Vector3d result;
	return result;
}

double GetZBufferDepth(Eigen::Vector3d const &position, Eigen::Vector3d const &direction,
											 double diameter, Camera const &cam,
											 std::function<void(const Renderer::shaderinfo_t *)> prepareDrawer,
											 std::function<void(const Renderer::shaderinfo_t *)> drawer)
{

	Renderer::shaderinfo_t shaderInfo = CreateShaderInfo("MouseSelector.vert", "MouseSelector.frag");

	prepareDrawer(&shaderInfo);

	QOpenGLFramebufferObjectFormat fboFormat;
	fboFormat.setSamples(0);	
	fboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
	
	std::unique_ptr<QOpenGLFramebufferObject> framebuffer;
	framebuffer.reset(new QOpenGLFramebufferObject(cam.pixel_width, cam.pixel_height,
		fboFormat));
	framebuffer->release();
	framebuffer->bind();

	glViewport(0, 0, cam.pixel_width, cam.pixel_height);

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	//auto aperture = ComputeAperture(viewport, diameter, cam.getFrustum()); // Bugged - wrong depth value
	
	Eigen::Vector2d aperture(5.0, 5.0);

	// position the pick point in the middle of the viewport
	Eigen::Vector2d pickPoint((viewport[2] + 1) / 2, (viewport[3] + 1) / 2);

	auto far_value=1.0f;
	glClearBufferfv(GL_DEPTH, 0, &far_value);
	GlPickInit(cam, pickPoint, aperture);

	GlModelView (cam, direction, position);

	GLdouble modelMatrix[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	GLdouble projectionMatrix[16];
	glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);

	drawer(&shaderInfo);

	glFlush();
	glFinish();


	Eigen::Vector2d readPoint(
			{pickPoint.x() - (aperture.x() / 2.0), pickPoint.y() - (aperture.y() / 2.0)});

	const long bufferSize = aperture.x() * aperture.y(); // problem - garbage being calculated for an ortho view
	std::unique_ptr<GLfloat[]> depthBuffer(new GLfloat[bufferSize]);

	glReadPixels(readPoint.x(), readPoint.y(), aperture.x(), aperture.y(), GL_DEPTH_COMPONENT,
							 GL_FLOAT, depthBuffer.get());

    //QImage image = framebuffer->toImage();
    //image.save(R"(D:\Users\mbonk\Pictures\openscad\image.jpg)");	

	glMatrixMode(GL_PROJECTION); // select the project matrix stack
	glPopMatrix();               // pop the previous project matrix

	glMatrixMode(GL_MODELVIEW); // select the project matrix stack
	glPopMatrix();              // pop the previous modelview matrix

	glPopAttrib();
	glFinish();

	framebuffer->release();

#if DumpHitBuffer
	std::vector<RGBQUAD> bits;
	// calculate size and align to a DWORD , we are assuming there is only one plane.
	size_t size = (static_cast<int>(aperture.x() * (sizeof(RGBQUAD) * 8) + 31) & -31) * aperture.y();
	bits.resize(size);

	double nearD = 0.1 * cam.zoomValue();
	double farD = 100 * cam.zoomValue();
	for (int j = 0; j < bufferSize; ++j) {
		// Linearize the depthbuffer
		float ndc = depthBuffer[j] * 2 -1;
		float linearDepth = (2.0 * nearD * farD) / (farD + nearD - ndc * (farD - nearD));
		BYTE c = static_cast<BYTE>(255.0 * (1.0 - linearDepth / farD));
		bits[j] = {c,c,c,0xff};
	}

	SaveAsBitmap(L"GetZBufferDepth.bmp", aperture.x(), aperture.y(), bits);
#endif
	
	int i, pos = 0;
	GLfloat depth = depthBuffer[pos];

	for (i = 1; i < bufferSize; ++i) {
		if (depthBuffer[i] < depth) {
			pos = i;
			depth = depthBuffer[pos];
		}
	}
	if (depth > 0.999) {
		return -1.0;
	}

	size_t y = pos / aperture.x();
	size_t x = pos - y * aperture.x();
	GLdouble objx, objy, objz;
	gluUnProject(static_cast<GLdouble>(readPoint.x() + x), static_cast<GLdouble>(readPoint.y() + y),
							 static_cast<GLdouble>(depth), modelMatrix, projectionMatrix, viewport,
							 &objx, &objy, &objz);
	Eigen::Vector3d hit(
			{static_cast<double>(objx), static_cast<double>(objy), static_cast<double>(objz)});

	return (hit - position).dot(direction);
}


Eigen::Vector3d GetCursorWorldCoordinates( Camera const &cam, Eigen::Vector2d const& mousePos) {
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	GLdouble projectionMatrix[16];
	glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);

	Eigen::Affine3d affine = cam.getAffine();
	Eigen::Affine3d modelView = affine.inverse();

  	GLdouble objx, objy, objz;
  	gluUnProject(mousePos.x(),mousePos.y(), 0,
               modelView.data(), projectionMatrix, viewport, &objx, &objy,
               &objz);

	// std::cout << "(" <<globalCursorPos.rx() << "," <<globalCursorPos.ry() << ")" << std::endl;
	return {objx,objy,objz};
}
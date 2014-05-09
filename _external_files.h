#ifndef _EXTERNAL_FILES_H
#define _EXTERNAL_FILES_H

// author: yangang wang
// 2013/03/01

//--------------------------------------------------------------
// add lib here ------------------------------------------------
// opengl
#pragma comment(lib, "freeglut.lib")
#pragma comment(lib, "glew32.lib")

// opencv
#if _DEBUG
#pragma comment(lib, "opencv_core245d.lib")
#pragma comment(lib, "opencv_ml245d.lib")
#pragma comment(lib, "opencv_highgui245d.lib")
#pragma comment(lib, "opencv_imgproc245d.lib")
#pragma comment(lib, "opencv_features2d245d.lib")
#pragma comment(lib, "opencv_flann245d.lib")
#pragma comment(lib, "opencv_nonfree245d.lib")
#else
#pragma comment(lib, "opencv_core245.lib")
#pragma comment(lib, "opencv_ml245.lib")
#pragma comment(lib, "opencv_highgui245.lib")
#pragma comment(lib, "opencv_imgproc245.lib")
#pragma comment(lib, "opencv_features2d245.lib")
#pragma comment(lib, "opencv_flann245.lib")
#pragma comment(lib, "opencv_nonfree245.lib")
#pragma comment(lib, "opencv_calib3d245.lib")
#endif

//--------------------------------------------------------------
// header files ------------------------------------------------
// OpenGL
#include "GL/glew.h"
#include "GL/glut.h"

// OpenCV
#include "opencv.hpp"

// Eigen
#include <Eigen/Eigen>
#include <ctime>
#include <math.h>

//--------------------------------------------------------------
// namespace ---------------------------------------------------
using namespace std;
using namespace Eigen;
using namespace cv;

// camera
typedef struct
{
	float Mmatrix[16];	// GL_MODELVIEW matrix
	float Pmatrix[16];	// GL_PROJECTION matrix
} GeoCastData;
extern vector<GeoCastData> camera;
extern int rect_width;
extern int rect_height;

#endif
#ifndef _EXTERNAL_FILES_H
#define _EXTERNAL_FILES_H
#define _USE_MATH_DEFINES
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
#pragma comment(lib, "opencv_video245d.lib")
#pragma comment(lib, "OpenMeshCored.lib");
#else
#pragma comment(lib, "opencv_core245.lib")
#pragma comment(lib, "opencv_ml245.lib")
#pragma comment(lib, "opencv_highgui245.lib")
#pragma comment(lib, "opencv_imgproc245.lib")
#pragma comment(lib, "opencv_features2d245.lib")
#pragma comment(lib, "opencv_flann245.lib")
#pragma comment(lib, "opencv_nonfree245.lib")
#pragma comment(lib, "opencv_calib3d245.lib")
#pragma comment(lib, "opencv_video245.lib")
#pragma comment(lib, "OpenMeshCore.lib")
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

//OpenMesh
#include <OpenMesh\Core\IO\MeshIO.hh>
#include <OpenMesh\Core\Mesh\TriMesh_ArrayKernelT.hh>
//--------------------------------------------------------------
// namespace ---------------------------------------------------
using namespace std;
using namespace Eigen;
using namespace cv;

//texture coordinate
typedef struct texturecoord{
	int u1,u2,v1,v2,pic1,pic2,u_d,v_d;
	float weight1,weight2;
	texturecoord():u1(-1),v1(-1),u2(-1),v2(-1),pic1(-1),pic2(-1),u_d(-1),v_d(-1),weight1(0.0),weight2(0.0){}
}TEXCOORD;
typedef OpenMesh::TriMesh_ArrayKernelT<> TriMesh;
extern int rect_width;
extern int rect_height;

#endif
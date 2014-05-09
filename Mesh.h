/**********************************************************/
/* Author: yan-gang wang, Tsinghua Unvi. Beijing, China.***/
/* E-mail: wang-yg09@mails.tsinghua.edu.cn ****************/
/* Date: 2011/04/18 ***************************************/
/**********************************************************/
#include "_external_files.h"
#ifndef _MESH_H
#define _MESH_H

struct MeshTexture
{
	MeshTexture():PicIndex1(-1),PicIndex2(-1),weight1(1.0),weight2(0.0),u1(0),v1(0),u2(0),v2(0){;};

	int PicIndex1;
	int PicIndex2;

	float weight1;
	float weight2;
	// texture coordinate
	int u1;
	int v1;
	int u2;
	int v2;
};

struct cameraPara
{
    Matrix4f intrinsic;
    Matrix4f external;
};

struct MeshVertex
{
	MeshVertex() : edge(-1){}
	Vector3f pos;
	Vector3f normal;
	Vector3f color;

	int edge;
    
    vector<double> weight;
	vector<Vector2f> texture;
	vector<bool> visible;
    vector<float> angle;
	vector<int> edges;
	vector<MeshTexture> texPic;

	~MeshVertex() {weight.clear(); texture.clear(); angle.clear();texPic.clear();}
};

struct MeshEdge
{
	MeshEdge() : vertex(-1), prev(-1){}

	int vertex;
	int prev;

	MeshTexture texMap;

	~MeshEdge() {;}
};


struct SkeletonJoint
{
	int curr;
	int prev;
	Vector3f pos;
	Vector3f norm;
};

struct JointRotate
{
	int index;
	double angle;
};

struct SWeight
{
	double weight;
	int index;
};
struct PColor
{
	double color[3];
	double nums;
};

class Mesh
{
public:
	Mesh(){}
	Mesh(const Mesh& m);
	Mesh operator=(const Mesh& m);
	Mesh(const string& file, int camNum=0);
    Mesh(const string file, char *cameraPath, int camNum = 0);

	~Mesh() {
		vertices.clear();
		edges.clear();
		skeleton.clear();
	}

	void computeVertexNormals();
	void computeCenter();
	void writeoff(const string& file);
	void writedmesh(const string& file);

	int getJoints() const{return (int)skeleton.size();};
	float getSceneSize() const{return scene_size;};
	int getPointNum() const{return (int)vertices.size();};
	const Vector3f getMeshCenter() const{return center;};
	const vector<MeshVertex>& getVertex() const{return vertices;};
	Vector3f computeBBox();
	Vector3f computeBBoxCenter();

private:
	void ReadOff(istream& strm, int camNums);
	void ReadMesh(istream& strm, int camNums);
	vector<SkeletonJoint> getskel(){return skeleton;}

public:
	vector<MeshVertex> vertices;
	vector<MeshEdge> edges;
	vector<SkeletonJoint> skeleton;
	vector<Vector2f> img_rect;
    vector<cameraPara> cameraParameters;

	Vector3f center;
	float scene_size;
};


#endif
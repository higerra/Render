/**********************************************************/
/* Author: yan-gang wang, Tsinghua Unvi. Beijing, China.***/
/* E-mail: wang-yg09@mails.tsinghua.edu.cn ****************/
/* Date: 2011/04/18 ***************************************/
/**********************************************************/
#include "Mesh.h"
#include <stdio.h>
#include <fstream>

Mesh::Mesh(const string& file, int camNum/*=0*/)
{
#ifndef _EXEC_FAST
	printf("%s%s","Reading ",file.c_str());
#endif

	ifstream obj(file.c_str());

	if(!obj.is_open()) { printf("can not read the mesh file!\n"); return;}

	if(file.length() < 4)
		return;

	if(string(file.end() - 4, file.end()) == string(".off"))
		ReadOff(obj,camNum);     
	else if(string(file.end() - 5, file.end()) == string(".mesh"))
		ReadMesh(obj, camNum);
	else if(string(file.end() - 6, file.end()) == string(".dmesh"))
		ReadMesh(obj,camNum);
	else
		return;

	obj.close();

	int verts = int(vertices.size());
	if(verts == 0)
		return;

	for(int i = 0; i < (int)edges.size(); ++i)					//保证所有的顶点都是有效的
	{ 
		if(edges.at(i).vertex<0||edges.at(i).vertex>=verts)
		{
			vertices.clear(); 
			edges.clear(); 
			cout<<"error!"<<i<<endl;
			return;
		}
	}

	computeVertexNormals();
#ifndef _EXEC_FAST
	printf(" success!\n");
#endif
}


Mesh::Mesh(const Mesh &m)
{
	// vector assign
	vertices.assign(m.vertices.begin(),m.vertices.end());
	edges.assign(m.edges.begin(),m.edges.end());
	skeleton.assign(m.skeleton.begin(),m.skeleton.end());
	img_rect.assign(m.img_rect.begin(),m.img_rect.end());

	// others
	center = m.center;
	scene_size = m.scene_size;
}

Mesh Mesh::operator=(const Mesh& m)
{
	if(&m!=this) {

		// vector assign
		vertices.assign(m.vertices.begin(),m.vertices.end());
		edges.assign(m.edges.begin(),m.edges.end());
		skeleton.assign(m.skeleton.begin(),m.skeleton.end());
		img_rect.assign(m.img_rect.begin(),m.img_rect.end());
		// others
		center = m.center;
		scene_size = m.scene_size;
	}

	return *this;
}

void Mesh::ReadOff(istream& strm,int camNums)
{
	string tmp;
	strm>>tmp;
	
	/*if(strcmp(tmp.c_str(),"COFF"));
	{
		cout<<".off file has been wrong, please check your file!"<<endl;
		return;
	}*/

	int vertex_numbers, face_numbers, tmp_number;
	strm>>vertex_numbers>>face_numbers>>tmp_number;

	double x,y,z;
	float r,g,b,a;
	for(int i=0;i<vertex_numbers;i++)	
	{
		strm>>x>>y>>z>>r>>g>>b>>a;
		vertices.resize(vertices.size() + 1);
		vertices.back().pos = Vector3f(x,y,z);
		vertices.back().color = Vector3f(r,g,b);
		vertices[i].texture.resize(camNums*20);
	}

	int index_v1,index_v2,index_v3;
	for(int i=0;i<face_numbers;i++)
	{
		int first = (int)edges.size();
		strm>>tmp_number>>index_v1>>index_v2>>index_v3;
		edges.resize(edges.size() + 3);
		edges[first + 0].vertex = index_v1;
		edges[first + 1].vertex = index_v2;
		edges[first + 2].vertex = index_v3;	
		
		edges[first + 0].prev = index_v3;
		edges[first + 1].prev = index_v1;
		edges[first + 2].prev = index_v2;

		vertices[index_v1].edges.push_back(first);
		vertices[index_v2].edges.push_back(first);
		vertices[index_v3].edges.push_back(first);
	}

	skeleton.clear();

	computeCenter();
	computeBBox();
	return;
}

void Mesh::ReadMesh(istream& strm, int camNums)
{
	int vertex_numbers, face_numbers, Joint_number, weights_number;
	strm>>vertex_numbers>>face_numbers>>Joint_number>>weights_number;

	// vertexes and weights
	double x,y,z;
	
	vertices.resize(vertex_numbers);
	for(int i=0;i<vertex_numbers;i++) {
		strm>>x>>y>>z;
		vertices[i].pos = Vector3f(x,y,z);
		vertices[i].weight.resize(Joint_number+1,.0);
		for(int j=0;j<weights_number;j++) {
			int index;
			double w;
			strm>>index>>w;
			vertices[i].weight[index] = w;
		}

		// color
		vertices[i].color[0] = 0.f;
		vertices[i].color[1] = 0.f;
		vertices[i].color[2] = 0.f;

		vertices[i].texture.resize(camNums*20);
	}

	// faces
	int index_v1,index_v2,index_v3;
	for(int i=0;i<face_numbers;i++)	{
		int first = (int)edges.size();
		strm>>index_v1>>index_v2>>index_v3;
		edges.resize(edges.size() + 3);
		edges[first + 0].vertex = index_v1;
		edges[first + 1].vertex = index_v2;
		edges[first + 2].vertex = index_v3;	

		edges[first + 0].prev = index_v3;
		edges[first + 1].prev = index_v1;
		edges[first + 2].prev = index_v2;

		vertices[index_v1].edges.push_back(first);
		vertices[index_v2].edges.push_back(first);
		vertices[index_v3].edges.push_back(first);
	}

	//skeleton
	skeleton.resize(Joint_number);
	for(int i=0;i<Joint_number;i++)
		strm>>skeleton[i].curr>>skeleton[i].norm[0]>>skeleton[i].norm[1]>>skeleton[i].norm[2]>>
			skeleton[i].pos[0]>>skeleton[i].pos[1]>>skeleton[i].pos[2]>>skeleton[i].prev;

	computeCenter();
	computeBBox();
	return;
}

void Mesh::writeoff(const string& file)
{
	ofstream obj(file.c_str());

	if(!obj.is_open()||(file.length() < 4)) {
		printf(" can not write the file!\n");
		return;
	}

	if(string(file.end() - 4, file.end()) != string(".off")) {
		printf(" the file is not .off, please check!\n");
		return;
	}

	obj<<"COFF"<<endl;
	obj<<(int)vertices.size()<<" "<<(int)edges.size()/3<<" "<<0<<endl;

	for(int i=0;i<(int)vertices.size();i++)
    {
		obj<<vertices[i].pos[0]<<' '<<vertices[i].pos[1]<<' '<<vertices[i].pos[2]<<' ';
        obj<<vertices[i].color[0]<<' '<<vertices[i].color[1]<<' '<<vertices[i].color[2]<<endl;
    }

	for(int i=0;i<(int)edges.size();i=i+3)
		obj<<3<<" "<<edges[i].vertex<<" "<<edges[i+1].vertex<<" "<<edges[i+2].vertex<<endl;

	return;
}

void Mesh::writedmesh(const string& file)
{
	ofstream obj(file.c_str());

	if(!obj.is_open()||(file.length() < 5)) {
		printf(" can not write the file!\n");
		return;
	}

	if(string(file.end() - 6, file.end()) != string(".dmesh")) {
		printf(" the file is not .off, please check!\n");
		return;
	}

	obj<<(int)vertices.size()<<" "<<(int)edges.size()/3<<" "<<skeleton.size()<<" "<<skeleton.size()+1<<endl;

	for(int i=0;i<(int)vertices.size();i++) {
		obj<<vertices[i].pos[0]<<" "<<vertices[i].pos[1]<<" "<<vertices[i].pos[2]<<" ";
		for(int j=0;j<(int)vertices[i].weight.size();j++)
			obj<<j<<" "<<vertices[i].weight[j]<<" ";
		obj<<endl;
	}

	for(int i=0;i<(int)edges.size();i=i+3)
		obj<<edges[i].vertex<<" "<<edges[i+1].vertex<<" "<<edges[i+2].vertex<<endl;
	for(int i=0;i<(int)skeleton.size();i++) {
		obj<<skeleton[i].curr<<" "<<skeleton[i].norm[0]<<" "<<skeleton[i].norm[1]<<" "<<skeleton[i].norm[2]<<" "<<
			skeleton[i].pos[0]<<" "<<skeleton[i].pos[1]<<" "<<skeleton[i].pos[2]<<" "<<skeleton[i].prev<<endl;
	}

	return;
}

void Mesh::computeVertexNormals()
{
	for(int i = 0; i < (int)vertices.size(); ++i)
		vertices[i].normal = Vector3f::Zero();
	for(int i = 0; i < (int)edges.size(); i += 3) {
		int i1 = edges[i].vertex;
		int i2 = edges[i + 1].vertex;
		int i3 = edges[i + 2].vertex;
		Vector3f normal = ((vertices[i2].pos - vertices[i1].pos).cross(vertices[i3].pos - vertices[i1].pos)).normalized();
		vertices[i1].normal += normal;
		vertices[i2].normal += normal;
		vertices[i3].normal += normal;
	}
	for(int i = 0; i < (int)vertices.size(); ++i)
		vertices[i].normal.normalize();
}

void Mesh::computeCenter()
{
	Vector3f tmpcenter(0,0,0);
	for(int i=0;i<(int)vertices.size();i++)
		tmpcenter += vertices[i].pos;

	center = tmpcenter/(int)vertices.size();
}

Vector3f Mesh::computeBBox()
{
	float xmin=1000000,xmax=-1000000,ymin=1000000,ymax=-1000000,zmin=1000000,zmax=-1000000;
	for(int i=0;i<getPointNum();i++) {
		if(xmin>vertices[i].pos[0]) xmin=vertices[i].pos[0];
		if(xmax<vertices[i].pos[0]) xmax=vertices[i].pos[0];

		if(ymin>vertices[i].pos[1]) ymin=vertices[i].pos[1];
		if(ymax<vertices[i].pos[1]) ymax=vertices[i].pos[1];

		if(zmin>vertices[i].pos[2]) zmin=vertices[i].pos[2];
		if(zmax<vertices[i].pos[2]) zmax=vertices[i].pos[2];
	}

	Vector3f tmp((xmax-xmin),(ymax-ymin),(zmax-zmin));
	scene_size = tmp.maxCoeff();
	return tmp;
}

Vector3f Mesh::computeBBoxCenter()
{
	double xmin=1000000,xmax=-1000000,ymin=1000000,ymax=-1000000,zmin=1000000,zmax=-1000000;
	for(int i=0;i<getPointNum();i++) {
		if(xmin>vertices[i].pos[0]) xmin=vertices[i].pos[0];
		if(xmax<vertices[i].pos[0]) xmax=vertices[i].pos[0];

		if(ymin>vertices[i].pos[1]) ymin=vertices[i].pos[1];
		if(ymax<vertices[i].pos[1]) ymax=vertices[i].pos[1];

		if(zmin>vertices[i].pos[2]) zmin=vertices[i].pos[2];
		if(zmax<vertices[i].pos[2]) zmax=vertices[i].pos[2];
	}

	return Vector3f((xmin+xmax)/2.,(ymin+ymax)/2.,(zmin+zmax)/2.);
}
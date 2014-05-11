#include "Mesh.h"
#include "GLShader.h"
#include <fstream>
#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 480
#define camNum 100
#define frameInterval 5
#define dynNum 54
#define videoLen 360

bool is_dependent = false;

Mesh Object, hand;
int rect_width;
int rect_height;
string texPath;
string texPrefix;
int viewcount = camNum/frameInterval;		//different from 'camNum'
int curFrame = 0;
int nextFrame = 1;
Vector4f ImgCenter; //center of the image, used for calculate gluLookAt parameters

//////////////////////////////////////////////////////////////////////////
// OpenGL
static int screenWidth, screenHeight;
static int outImgWidth, outImgHeight;
static bool isWriting=false;
static int faceNum=0;
static int wringIdx=0;
static int textureIdx=0;
static string imsavePath;
static GLuint glutWindowHandle;
static GLuint fbo[2], rbo[3];
#define BUFFERNUM 6
GLuint bufferobject[BUFFERNUM];//0:index, 1: position, 2: texture coordinate 1, 3:texture coordinate 2, 4:texture blending weight
GLuint texture;
static int TexNum;
static Shader _shader;
#define BUFFER_OFFSET(i) ((uchar*)NULL + (i))

//for saving useage
vector <Mat> saveImage;
void save();

//for time-dependent rendering
Mat mask;
int maskwidth;
int maskheight;
Matrix4f referencePara;	//!! to be intergrated to mesh class
Matrix4f intrinsic;		//!! to be intergrated to mesh class
int dyn_curFrame = 0;
Vector2i getimagePoint(Vector3f worldPoint,int camId);
Vector2i getvideoPoint(Vector3f worldPoint);


void reshape(int w, int h);
void renderScene();
void getTexture(int curFrame,int nextFrame);

void data2GPU(const Mesh& m);
void OpenGLshow(int argc, char** argv, const Mesh& m);
void processSpecialKeys(int key, int x, int y);
void processNormalKeys(unsigned char key,int x,int y);
void close();

//for navigation
vector <Matrix4f,aligned_allocator<Matrix4f>> camPara;
void calculateDir(int cameraId,float *lookatMatrix);
void updateView();

static float difflookat[6] = {0.0,0.0,0.0,0.0,0.0,0.0};
static float currentlookat[6] = {};
static float lookattable[100][6] = {};
int move_count = 0;

char prefix[100];

//////////////////////////////////////////////////////////////////////////
// mouse
#include "GL/freeglut_ext.h"
#define TRACKBALLSIZE  ((float)3)

static Vector3f spin_center;
static Vector3f scene_center;

static float scene_size;
static float camModelView[16];
static float camProjView[16];
static int camviewport[4];

void main(int argc, char** argv)
{

	TexNum = dynNum+viewcount;
    sprintf(prefix,"D:/yanhang/RenderProject/room/data5.9_dyn_cap");

	char offpath[100],geopath[100],maskpath[100];
	sprintf(offpath,"%s/Mesh.off",prefix);
	sprintf(geopath,"%s/Geometry",prefix);
	sprintf(maskpath,"%s/image/frame_reference.png",prefix);

	//read the reference frame and its parameter !!to be modified or deleted
	mask = imread(maskpath);
	if(!mask.data)
	{
		cout<<"cannot read the mask!"<<endl;
		system("pause");
		exit(-1);
	}
	flip(mask,mask,1);
	cvtColor(mask,mask,CV_RGB2GRAY);
	maskwidth = mask.cols;
	maskheight = mask.rows;

	char buffer[100];
	sprintf(buffer,"%s/frame_reference.txt",geopath);
	ifstream camerafin(buffer);
    for(int y=0;y<4;y++)
    {
        for(int x=0;x<4;x++)
            camerafin>>referencePara(y,x);
    }
	camerafin.close();
	referencePara.transposeInPlace();

	intrinsic<<575,0.0,319.5,0.0,0.0,575,239.5,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0;

	hand = Mesh(string(offpath),viewcount);

	OpenGLshow(argc,argv,hand);
}

void save()
{
	cout<<"Saving..."<<endl;
	if(saveImage.size() == 0)
	{
		cout<<"Saving array is empty!"<<endl;
		return;
	}
	char buffer[100];
	sprintf(buffer,"%s/output.avi",prefix);
	VideoWriter vWriter(string(buffer),-1,12.0,saveImage[0].size(),1);
	for(int i=0;i<saveImage.size();i++)
	{
		vWriter<<saveImage[i];
		waitKey(10);
	}
	char isSaveFrame;
	cout<<"Save individual frame? (y/n)"<<endl;
	isSaveFrame = getchar();
	if(isSaveFrame == 'y')
	{
		for(int i=0;i<saveImage.size();i++)
		{
			char imgbuffer[100];
			sprintf(imgbuffer,"%s/saveFrame/frame%03d.png",prefix,i);
			imwrite(imgbuffer,saveImage[i]);
		}
	}
	cout<<"Save Complete!"<<endl;
}
//////////////////////////////////////////////////////////////////////////
// for showing
void data2GPU(const Mesh& m)
{
	// vertex position
	glBindBuffer(GL_ARRAY_BUFFER,bufferobject[1]);
	GLfloat* glVerPos = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER,GL_READ_WRITE);
#pragma omp parallel for
	for(int i=0;i<m.edges.size();i++) {
		int tmp = m.edges[i].vertex;
		glVerPos[i*3+0] = (GLfloat)m.getVertex()[tmp].pos[0];
		glVerPos[i*3+1] = (GLfloat)m.getVertex()[tmp].pos[1];
		glVerPos[i*3+2] = (GLfloat)m.getVertex()[tmp].pos[2];
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);

	//independent texture
	glBindBuffer(GL_ARRAY_BUFFER,bufferobject[5]);
	GLfloat* glVerIndTex = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER,GL_READ_WRITE);
#pragma omp parallel for
	for(int i=0;i<m.edges.size();i++) {
		int tmp = m.edges[i].vertex;
		glVerIndTex[i*4+0] = (GLfloat)(m.getVertex()[tmp].color[0]/255.0);
		glVerIndTex[i*4+1] = (GLfloat)(m.getVertex()[tmp].color[1]/255.0);
		glVerIndTex[i*4+2] = (GLfloat)(m.getVertex()[tmp].color[2]/255.0);
		glVerIndTex[i*4+3] = 1.0;
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);
}

void updateTexture()
{
	//texture coordinate 1
	glBindBuffer(GL_ARRAY_BUFFER,bufferobject[2]);
	GLfloat* glVerTex1 = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER,GL_WRITE_ONLY);
#pragma omp parallel for
	for(int i=0;i<hand.edges.size();i++) {
		glVerTex1[i*3+0] = (GLfloat)hand.edges[i].texMap.u1/float(outImgWidth);
		glVerTex1[i*3+1] = (GLfloat)hand.edges[i].texMap.v1/float(outImgHeight);
		glVerTex1[i*3+2] = (GLfloat)hand.edges[i].texMap.PicIndex1;
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);

	//texutre coordinate 2
	glBindBuffer(GL_ARRAY_BUFFER,bufferobject[3]);
	GLfloat* glVerTex2 = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER,GL_WRITE_ONLY);
#pragma omp parallel for
	for(int i=0;i<hand.edges.size();i++)
	{
		glVerTex2[i*3+0] = (GLfloat)hand.edges[i].texMap.u2/float(outImgWidth);
		glVerTex2[i*3+1] = (GLfloat)hand.edges[i].texMap.v2/float(outImgHeight);
		glVerTex2[i*3+2] = (GLfloat)hand.edges[i].texMap.PicIndex2;
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);

	//weight
	glBindBuffer(GL_ARRAY_BUFFER,bufferobject[4]);
	GLfloat* glVerBlending = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER,GL_WRITE_ONLY);
#pragma omp parallel for
	for(int i=0;i<hand.edges.size();i++)
	{
		glVerBlending[i*2+0] = (GLfloat)hand.edges[i].texMap.weight1;
		glVerBlending[i*2+1] = (GLfloat)hand.edges[i].texMap.weight2;
		if(hand.edges[i].texMap.PicIndex1 == -1)
			glVerBlending[i*2+0] = 0.0f;
		else if(hand.edges[i].texMap.PicIndex1 == -1)
			glVerBlending[i*2+1] = 0.0f;
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);
}

void renderScene()
{
	
	if(isWriting) {
		if(outImgWidth!=screenWidth||outImgHeight!=screenHeight) {
			glBindFramebuffer(GL_FRAMEBUFFER,fbo[0]);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glViewport(0,0,outImgWidth,outImgHeight);
		}
	}
	else {
		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glViewport(0,0,screenWidth,screenHeight);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	//glShadeModel(GL_FLAT);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_LIGHTING);

	_shader.EnableShader();

	GLuint Loc;
	glGetFloatv(GL_MODELVIEW_MATRIX,camModelView);
	Loc = glGetUniformLocation(_shader.GetProgramID(),"mv_mat");
	if(Loc==-1) printf("can not find mv_mat\n");
	glUniformMatrix4fv(Loc,1,false,camModelView);


	Loc = glGetUniformLocation(_shader.GetProgramID(),"mp_mat");
	if(Loc==-1) printf("can not find mp_mat\n");
	glUniformMatrix4fv(Loc,1,false,camProjView);

	Loc = glGetUniformLocation(_shader.GetProgramID(),"tex_array");
	if(Loc==-1) printf("can not find tex_array\n");
	glUniform1i(Loc,0); // Texture0
	Loc = glGetUniformLocation(_shader.GetProgramID(),"TexNum");
	glUniform1i(Loc,TexNum);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,bufferobject[0]);
	glDrawElements(GL_TRIANGLES,faceNum,GL_UNSIGNED_INT,0);

	_shader.DisableShader();

	if(isWriting && saveImage.size() <= videoLen) {			
		// the three sentences are referred to: http://www.opengl.org/wiki/GL_EXT_framebuffer_multisample
		// Yangang, 2013/02/26
		//
		//Bind the standard FBO
		glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, fbo[1]);
		//Let's say I want to copy the entire surface
		//Let's say I only want to copy the color buffer only
		//Let's say I don't need the GPU to do filtering since both surfaces have the same dimension
		glBlitFramebufferEXT(0, 0, outImgWidth, outImgHeight, 0, 0, outImgWidth, outImgHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		//--------------------
		//Bind the standard FBO for reading
		glBindFramebufferEXT(GL_FRAMEBUFFER, fbo[1]);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		cv::Mat ImgGL(outImgHeight,outImgWidth,CV_8UC3);
		glReadPixels(0,0,outImgWidth,outImgHeight,GL_BGR_EXT,GL_UNSIGNED_BYTE,ImgGL.data);
		cv::flip(ImgGL,ImgGL,0);

		saveImage.push_back(ImgGL);
		isWriting=false;
	}

	glFlush();
	glutSwapBuffers();
}

void idlefunc()	//used for animation
{
	waitKey(200);	//modulate the frame rate
	dyn_curFrame++;
	if(dyn_curFrame == dynNum)
		dyn_curFrame = 0;
	getTexture(curFrame,nextFrame);
	isWriting = true;
	renderScene();
	isWriting = false;
	renderScene();
}


void OpenGLshow(int argc, char** argv, const Mesh& m)
{
	//////////////////////////////////////////////////////////////////////////
	// Init glut
	screenWidth = 800, screenHeight = 600;
	outImgWidth = 640, outImgHeight= 480; //1440, 1080

	ImgCenter = Vector4f(static_cast<float>(outImgWidth/2),static_cast<float>(outImgHeight/2),1.0,0.0);

	glutInit(&argc,argv); 
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glutInitWindowSize(screenWidth,screenHeight);
	glutCreateWindow("Dmesh texture mapping rendering");

	if (glewInit() != GLEW_OK)
	{
		printf("Glew failed to initialize.");
		exit(-1);
	}
	if(GLEW_ARB_vertex_shader && GLEW_ARB_fragment_shader)
	{
		_shader.ReadVertextShader("texture_array.vert");
		_shader.ReadFragmentShader("texture_array.frag");
		_shader.SetShader();
	}
	
	//////////////////////////////////////////////////////////////////////////
	// Init OpenGL
	glEnable(GL_DEPTH);
	glClearColor(0,0,0,0);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

    
	
	//////////////////////////////////////////////////////////////////////////
	// generate the OpenGL buffer
	//
	// Frame buffer for off-line rendering
	// rendering usage
	glGenFramebuffers(2,fbo);
	glBindFramebuffer(GL_FRAMEBUFFER,fbo[0]);
	glGenRenderbuffers(3,rbo);
	glBindRenderbuffer(GL_RENDERBUFFER,rbo[0]);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER,5,GL_RGB,outImgWidth,outImgHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,rbo[0]);
	glBindRenderbuffer(GL_RENDERBUFFER,rbo[1]);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER,5,GL_DEPTH_STENCIL,outImgWidth,outImgHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_RENDERBUFFER,rbo[1]);
	// saving usage
	glBindFramebuffer(GL_FRAMEBUFFER,fbo[1]);
	glBindRenderbuffer(GL_RENDERBUFFER,rbo[2]);
	glRenderbufferStorage(GL_RENDERBUFFER,GL_RGB,outImgWidth,outImgHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,rbo[2]);
	// bind to the screen
	glBindFramebuffer(GL_FRAMEBUFFER,0);

	// texture mapping
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_3D);
	glGenTextures(1,&texture);
	glBindTexture(GL_TEXTURE_2D_ARRAY,texture);
	glTexParameteri(GL_TEXTURE_2D_ARRAY , GL_TEXTURE_MIN_FILTER , GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY , GL_TEXTURE_MAG_FILTER , GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY , GL_TEXTURE_WRAP_S , GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY , GL_TEXTURE_WRAP_T , GL_REPEAT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_RGB,outImgWidth,outImgHeight,TexNum,0,GL_BGR,GL_UNSIGNED_BYTE,NULL);

	char buffer[256];
	//read the dynamic area
	for(int i=0;i<dynNum;i++) 
	{
		//printf("reading texture...%d\n",i);
		sprintf(buffer,"%s/image/dynarea%03d.png",prefix,i);
		Mat TexImg = imread(buffer);
		if(!TexImg.data)
		{
			printf("can not read the image %s!\n",buffer);
			system("pause");
			exit(-1);
		}
		flip(TexImg,TexImg,1);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,0,i,TexImg.cols,TexImg.rows,1,GL_BGR,GL_UNSIGNED_BYTE,TexImg.data);
	}
	//read the textures & camera Parameters
	int texind = 0;
	for(int i=0; texind<camNum; i++)
	{
		sprintf(buffer,"%s/image/frame%03d.png",prefix,texind);
		Mat TexImg = imread(buffer);
		if(!TexImg.data)
		{
			printf("can not read the image %s!\n",buffer);
			system("pause");
			exit(-1);
		}
		flip(TexImg,TexImg,1);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,0,i+dynNum,TexImg.cols,TexImg.rows,1,GL_BGR,GL_UNSIGNED_BYTE,TexImg.data);

		sprintf(buffer,"%s/Geometry/frame%03d.txt",prefix,texind);
		ifstream fin(buffer);
		if(!fin)
		{
			printf("can not read the camera file: %s\n",buffer);
			system("pause");
			exit(-1);
		}
		Matrix4f temp;
		for(int y=0;y<4;y++)
        {
            for(int x=0;x<4;x++)
                fin>>temp(y,x);
        }
        temp(3,3) = 1;
		camPara.push_back(temp.transpose());
		texind = texind+frameInterval;
	}


	// Set camera
	scene_center = m.getMeshCenter();
	scene_size = m.getSceneSize();

	glViewport(0,0,screenWidth,screenHeight);
	glGetIntegerv(GL_VIEWPORT,camviewport);
    
    for(int i=0;i<viewcount;i++)
        calculateDir(i,lookattable[i]);
    
    for(int i=0;i<6;i++)
    {
        currentlookat[i] = lookattable[curFrame][i];
        difflookat[i] = (lookattable[nextFrame][i] - lookattable[curFrame][i])/10.0;
    }
    updateView();
    
    
    
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    //float znear = abs(scene_center[2])*0.01f;
	float znear = 0.001;
	float zfar = abs(scene_center[2]) + scene_size*30.f;
	gluPerspective(45.0,(float)screenWidth/(float)screenHeight,znear,zfar);
	glGetFloatv(GL_PROJECTION_MATRIX,camProjView);

	//////////////////////////////////////////////////////////////////////////

	glGenBuffers(BUFFERNUM,bufferobject);
	// face
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,bufferobject[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,m.edges.size()*sizeof(GLuint),NULL,GL_STATIC_DRAW);
	GLuint* glFace = (GLuint*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER,GL_WRITE_ONLY);
	faceNum = m.edges.size();
	for(int i=0;i<faceNum;i++) {glFace[i] = i;}
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

	// vertex position
	GLuint loc;
	glBindBuffer(GL_ARRAY_BUFFER,bufferobject[1]);
	glBufferData(GL_ARRAY_BUFFER,m.edges.size()*3*sizeof(GLfloat),NULL,GL_STATIC_DRAW);
	loc = glGetAttribLocation(_shader.GetProgramID(),"position");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc,3,GL_FLOAT,0,0,BUFFER_OFFSET(0));

	//texture coord 1
	glBindBuffer(GL_ARRAY_BUFFER,bufferobject[2]);
	glBufferData(GL_ARRAY_BUFFER,m.edges.size()*3*sizeof(GLfloat),NULL,GL_STATIC_DRAW);
	loc = glGetAttribLocation(_shader.GetProgramID(),"texcoord1");	//u,v,pic
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc,3,GL_FLOAT,0,0,BUFFER_OFFSET(0));

	//texture coord 2
	glBindBuffer(GL_ARRAY_BUFFER,bufferobject[3]);
	glBufferData(GL_ARRAY_BUFFER,m.edges.size()*3*sizeof(GLfloat),NULL,GL_STATIC_DRAW);
	loc = glGetAttribLocation(_shader.GetProgramID(),"texcoord2"); //u,v,pic
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc,3,GL_FLOAT,0,0,BUFFER_OFFSET(0));


	//blending weight
	glBindBuffer(GL_ARRAY_BUFFER,bufferobject[4]);
	glBufferData(GL_ARRAY_BUFFER,m.edges.size()*2*sizeof(GLfloat),NULL,GL_STATIC_DRAW);
	loc = glGetAttribLocation(_shader.GetProgramID(),"weight"); //weight
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc,2,GL_FLOAT,0,0,BUFFER_OFFSET(0));

	//view-independent color
	glBindBuffer(GL_ARRAY_BUFFER,bufferobject[5]);
	glBufferData(GL_ARRAY_BUFFER,m.edges.size()*4*sizeof(GLfloat),NULL,GL_STATIC_DRAW);
	loc = glGetAttribLocation(_shader.GetProgramID(),"mixcolor");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc,4,GL_FLOAT,0,0,BUFFER_OFFSET(0));

	data2GPU(m);

	getTexture(curFrame,nextFrame);

	glutReshapeFunc(reshape);
	glutSpecialFunc(processSpecialKeys);
	glutKeyboardFunc(processNormalKeys);
	glutDisplayFunc(renderScene);
	glutIdleFunc(idlefunc);
	glutCloseFunc(close); //delete shader source
	glutMainLoop();

	glDeleteBuffers(BUFFERNUM,bufferobject);
	glDeleteTextures(1,&texture);
}

void reshape(int w, int h)
{ 
	if(h==0) h=1;
	screenWidth = w;
	screenHeight = h;
	glViewport(0,0,screenWidth,screenHeight);
	glGetIntegerv(GL_VIEWPORT,camviewport);
} 

//////////////////////////////////////////////////////////////////////////

void calculateDir(int cameraId,float *lookatMatrix)
{
    Matrix4f extr = camPara[cameraId];
    Matrix4f intr = intrinsic;
    
    Vector4f imgCenter_world_homo = extr.inverse()*intr.inverse()*ImgCenter;

    imgCenter_world_homo(3) = 1;

    for(int i=0;i<3;i++)
        lookatMatrix[i] = extr(i,3);
    for(int i=3;i<6;i++)
        lookatMatrix[i] = imgCenter_world_homo(i-3) - extr(i-3,3);
}

void updateView()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt((GLdouble)currentlookat[0], -1*(GLdouble)currentlookat[1], (GLdouble)currentlookat[2], (GLdouble)currentlookat[3], (GLdouble)currentlookat[4], (GLdouble)currentlookat[5], 0.0, -1.0, 0.0);
}

void processNormalKeys(unsigned char key,int x,int y)
{
	if(key == 27)
	{
		save();
		exit(0);
	}
	if(key == 'd')
		is_dependent = !is_dependent;
}

void processSpecialKeys(int key, int x, int y)
{
    switch(key)
    {
        case GLUT_KEY_RIGHT:
            if(move_count == 9 && nextFrame < viewcount-1)
            {
                curFrame ++;
                nextFrame ++;

                for(int i=0;i<6;i++)
                    currentlookat[i] = lookattable[curFrame][i];
                for(int i=0;i<6;i++)
                    difflookat[i] = (lookattable[nextFrame][i] - lookattable[curFrame][i])/10.0;
                move_count = 0;
            }else{
				for(int i=0;i<6;i++)
					currentlookat[i] += difflookat[i];
				move_count++;
			}
            updateView();
			if(is_dependent) getTexture(curFrame,nextFrame);
            glutPostRedisplay();
            break;
        case GLUT_KEY_LEFT:
            if(curFrame == 0)
                break;
            curFrame -= 5;
            nextFrame -= 5;
			getTexture(curFrame,nextFrame);
            for(int i=0;i<6;i++)
                currentlookat[i] = lookattable[curFrame][i];
            for(int i=0;i<6;i++)
                difflookat[i] = (lookattable[nextFrame][i] - lookattable[curFrame][i])/10.0;
            move_count = 0;
            updateView();
            glutPostRedisplay();
            break;
		case GLUT_KEY_UP:
			currentlookat[4] += 0.01;
			updateView();
			glutPostRedisplay();
			break;
		case GLUT_KEY_DOWN:
			currentlookat[4] -= 0.01;
			updateView();
			glutPostRedisplay();
			break;
    }
	cout<<"curFrame: "<<curFrame<<endl;
}

void close()
{
	_shader.deleteshader();
}

//////////////////////////////////////////////////////////////////////////
// mouse


bool isValid(Vector2i imagePoint)
{
	if(imagePoint[0]>=0 && imagePoint[0]<IMAGE_WIDTH
		&& imagePoint[1]>=0 && imagePoint[1]<IMAGE_HEIGHT)
		return 1;
	else 
		return 0;
}

Vector2i getimagePoint(Vector3f worldPoint,int camId)
{
	Matrix4f intri = intrinsic;
	Matrix4f exter = camPara[camId];

	Vector4f worldPoint_homo(worldPoint[0],worldPoint[1],worldPoint[2],1);
	Vector4f imagePoint_homo = intri*exter*worldPoint_homo;
	Vector2i imagePoint(static_cast<int>(imagePoint_homo[0]/imagePoint_homo[2]),static_cast<int>(imagePoint_homo[1]/imagePoint_homo[2]));

	return imagePoint;
}

Vector2i getVideoPoint(Vector3f worldPoint)
{
	Matrix4f intri = intrinsic;
	Matrix4f exter = referencePara;

	Vector4f worldPoint_homo(worldPoint[0],worldPoint[1],worldPoint[2],1);

	Vector4f imagePoint_homo = intri*exter*worldPoint_homo;
	Vector2i imagePoint(static_cast<int>(imagePoint_homo[0]/imagePoint_homo[2]),static_cast<int>(imagePoint_homo[1]/imagePoint_homo[2]));
	return imagePoint;
}

float distance(int camId)
{
	float res = powf((currentlookat[0]-camPara[camId](0,3))*(currentlookat[0]-camPara[camId](0,3)) + 
		(currentlookat[1]-camPara[camId](1,3))*(currentlookat[1]-camPara[camId](1,3)) + 
		(currentlookat[2]-camPara[camId](2,3))*(currentlookat[2]-camPara[camId](2,3)),0.5);
	return res;
}

void getTexture(int cur,int next)
{
	for(int i=0;i<hand.edges.size();i++)
	{
		int tmp = hand.edges[i].vertex;
		
		Vector2i videoPoint = getVideoPoint(hand.getVertex()[tmp].pos);

		Vector2i imagePoint1 = getimagePoint(hand.getVertex()[tmp].pos,cur);
		Vector2i imagePoint2 = getimagePoint(hand.getVertex()[tmp].pos,next);
		
		hand.edges[i].texMap.weight1 = 0.0;
		hand.edges[i].texMap.weight2 = 0.0;

		if(is_dependent)
		{
			float weight_1 = distance(next);
			float weight_2 = distance(cur);
			if(isValid(imagePoint1))
			{
				hand.edges[i].texMap.u1 = imagePoint1[0];
				hand.edges[i].texMap.v1 = imagePoint1[1];
				hand.edges[i].texMap.PicIndex1 = cur + dynNum;
				hand.edges[i].texMap.weight1 = weight_1;

			}else
				hand.edges[i].texMap.weight1 = 0.0;

			if(isValid(imagePoint2))
			{
				hand.edges[i].texMap.u2 = imagePoint2[0];
				hand.edges[i].texMap.v2 = imagePoint2[1];
				hand.edges[i].texMap.PicIndex2 = next + dynNum;
				hand.edges[i].texMap.weight2 = weight_2;
			}else
				hand.edges[i].texMap.weight2 = 0.0;

			float sumweight =hand.edges[i].texMap.weight2 + hand.edges[i].texMap.weight1;
			if(sumweight != 0)
			{
				hand.edges[i].texMap.weight2 = hand.edges[i].texMap.weight2/sumweight;
				hand.edges[i].texMap.weight1 = hand.edges[i].texMap.weight1/sumweight;
			}
		}

		if(isValid(videoPoint))
		{
			int locmask = static_cast<int>(mask.at<uchar>(videoPoint[1],videoPoint[0]));
			if(locmask == 255)
			{
				hand.edges[i].texMap.weight2 = 0.0;
				hand.edges[i].texMap.weight1 = 0.0;
				hand.edges[i].texMap.u1 = videoPoint[0];
				hand.edges[i].texMap.v1 = videoPoint[1];
				hand.edges[i].texMap.PicIndex1 = dyn_curFrame;
				hand.edges[i].texMap.weight1 = 1.0;
			}
		}
	}

	updateTexture();
	glutPostRedisplay();
}

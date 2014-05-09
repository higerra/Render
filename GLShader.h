// Yangang Wang
// 2013/03/14
//

#ifndef __GLSHADER_H_
#define __GLSHADER_H_

#include <windows.h>
#include <stdio.h>
#include <GL/glew.h>

#pragma warning(disable:4267)
#pragma warning(disable:4996)

class Shader
{
public:
	Shader();
	~Shader(){;};

	bool ReadShader(char *vert, char *frag);
	bool ReadVertextShader(char *_fn);
	bool ReadFragmentShader(char *_fn);

	void SetShader();

	void EnableShader();
	void DisableShader();

	void deleteshader();

	GLuint GetProgramID(){return m_Program;};
private:
	bool TextFileRead(char *_fn,GLchar *&_shader);
	GLchar *m_VertexShader;
	GLchar *m_FragmentShader;

	GLuint m_Program;
	GLuint m_Vert,m_Frag;
};

#endif
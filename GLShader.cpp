// Yangang Wang
// 2013/03/14
//

#include "GLShader.h"

Shader::Shader()
{
	this->m_VertexShader = NULL;
	this->m_FragmentShader = NULL;
	this->m_Program = NULL;
	this->m_Frag = NULL;
	this->m_Vert = NULL;
}

void Shader::deleteshader()
{
	glDetachShader(m_Program,m_Vert);
	glDetachShader(m_Program,m_Frag);

	glDeleteShader(m_Vert);
	glDeleteShader(m_Frag);
	glDeleteProgram(m_Program);

	if(NULL != m_VertexShader)
		delete[] m_VertexShader;
	if(NULL != m_FragmentShader)
		delete[] m_FragmentShader;
}

bool Shader::TextFileRead(char *_fn, GLchar *&_shader)
{
	if(NULL == _fn)
		return false;

	FILE *fp;
	int count = 0;

	fp = fopen(_fn,"rt");
	if(NULL == fp)
		return false;

	fseek(fp,0,SEEK_END);
	count = ftell(fp);
	rewind(fp);

	if(count<=0) return false;

	_shader = new GLchar[count];
	count = fread(_shader,sizeof(GLchar),count,fp);
	_shader[count] = '\0';
	fclose(fp);

	return true;
}

bool Shader::ReadShader(char *vert, char *frag)
{
	if(ReadVertextShader(vert)&ReadFragmentShader(frag)) return true;
	else return false;
}

bool Shader::ReadVertextShader(char *_fn)
{
	if(TextFileRead(_fn,m_VertexShader)) return true;
	else return false;
}

bool Shader::ReadFragmentShader(char *_fn)
{
	if(TextFileRead(_fn,m_FragmentShader)) return true;
	else return false;
}

void Shader::SetShader()
{
	GLint success;

	m_Program = glCreateProgram();
	m_Vert = glCreateShader(GL_VERTEX_SHADER);
	m_Frag = glCreateShader(GL_FRAGMENT_SHADER);

	const GLchar *vv = m_VertexShader;
	const GLchar *ff = m_FragmentShader;
	int vLength = strlen(vv);
	int fLength = strlen(ff);
	glShaderSource(m_Vert,1,&vv,&vLength);
	glShaderSource(m_Frag,1,&ff,&fLength);

	glCompileShader(m_Vert);
	glCompileShader(m_Frag); 
	glGetShaderiv(m_Vert, GL_COMPILE_STATUS, &success); 
	if (!success) { 
		printf("Vertex shader:\n");
		GLchar InfoLog[1024]; 
		glGetShaderInfoLog(m_Vert, sizeof(InfoLog), NULL, InfoLog); 
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", GL_VERTEX_SHADER, InfoLog); 
		system("pause");
	}
	glGetShaderiv(m_Frag, GL_COMPILE_STATUS, &success); 
	if (!success) { 
		printf("Fragment shader:\n");
		GLchar InfoLog[1024]; 
		glGetShaderInfoLog(m_Frag, sizeof(InfoLog), NULL, InfoLog); 
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", GL_FRAGMENT_SHADER, InfoLog); 
		system("pause");
	}

	glAttachShader(m_Program,m_Vert);
	glAttachShader(m_Program,m_Frag);

	//here, we can use glBindAttribLocation
	//example:
	//	glBindAttribLocation(m_Program,0,"XXX"); //XXX is the 'in' variable in vertex shader file

	glLinkProgram(m_Program);
	glGetProgramiv(m_Program, GL_LINK_STATUS, &success); 
	if (success == 0) { 
		GLchar ErrorLog[1024];
		glGetProgramInfoLog(m_Program, sizeof(ErrorLog), NULL, ErrorLog); 
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog); 
		system("pause");
	}
}

void Shader::EnableShader()
{
	glUseProgram(m_Program);
}

void Shader::DisableShader()
{
	glUseProgram(0);
}
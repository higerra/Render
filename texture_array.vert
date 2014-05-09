#version 330 
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 texcoord1;
layout(location = 2) in vec3 texcoord2;
layout(location = 3) in vec2 weight;
layout(location = 4) in vec4 mixcolor;

// uniform varaibles
uniform mat4 mv_mat;
uniform mat4 mp_mat;

// to fragment shader
out vec3 frag_tex1;
out vec3 frag_tex2;
out vec2 frag_weight;
out vec4 frag_mixcolor;

void main()
{
    gl_Position = mp_mat * mv_mat * vec4(position,1.0);

	frag_tex1 = texcoord1;
	frag_tex2 = texcoord2;
	frag_weight = weight;
	frag_mixcolor = mixcolor;
}
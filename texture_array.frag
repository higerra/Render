#version 330
#extension GL_EXT_texture_array: enable

uniform sampler2DArray tex_array;
uniform int TexNum;

in vec3 frag_tex1;
in vec3 frag_tex2;
in vec2 frag_weight;
in vec4 frag_mixcolor;

out vec4 frag_color;
void main()
{

    float actual_layer1 = max(0, min(TexNum-1, floor(frag_tex1.z+0.5)));
	float actual_layer2 = max(0, min(TexNum-1, floor(frag_tex2.z+0.5)));

    vec3 texCoord1 = vec3(frag_tex1.xy,actual_layer1);
	vec3 texCoord2 = vec3(frag_tex2.xy,actual_layer2);
    
	if(frag_weight.s == 0.0 && frag_weight.t == 0.0)
	{
		frag_color = frag_mixcolor;
	}else{
		frag_color = frag_weight.s*texture2DArray(tex_array,texCoord1.xyz) + frag_weight.t*texture2DArray(tex_array,texCoord2.xyz);
	}
}
# version 330 core
in vec3 pos;
in vec2 texcoord;
in vec3 normal;


uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

//uniform mat4 NM;

// varying vec4 V_Color;
varying vec2 V_Texcoord;

void main()
{
    vec3 a1 = normal;
    V_Texcoord = texcoord;
    gl_Position=P*V*M*vec4(pos,1.0);
}
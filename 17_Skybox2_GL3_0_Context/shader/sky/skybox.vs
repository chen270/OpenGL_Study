#version 330 core
in vec3 pos;
// attribute vec2 texcoord;
// attribute vec3 normal;


uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

out vec3 V_Texcoord;

void main()
{
    V_Texcoord = pos;
    gl_Position=P*V*M*vec4(pos,1.0);
}
#version 330 core
in vec3 pos;
in vec4 color;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

out vec4 V_Color;

void main()
{
    gl_Position=P*V*M*vec4(pos,1.0);
    V_Color = color; // 不使用 color 的话，可能会被编译器优化掉
}
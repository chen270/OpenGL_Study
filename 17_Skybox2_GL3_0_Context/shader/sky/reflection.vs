#version 330 core
in vec3 pos;
// attribute vec2 texcoord;
in vec3 normal;


uniform mat4 M;
uniform mat4 V;
uniform mat4 P;
uniform mat4 NM;

// out vec3 V_Texcoord;
out vec4 V_WorldPos;
out vec3 V_Normal;

void main()
{
    // V_Texcoord = pos;
    V_WorldPos = M * vec4(pos, 1); // 世界坐标系
    V_Normal = mat3(NM) * normal; // 法线

    gl_Position=P*V*M*vec4(pos,1.0);
}
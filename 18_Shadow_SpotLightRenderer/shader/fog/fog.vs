# version 330 core

attribute vec3 pos;
attribute vec2 texcoord;
attribute vec3 normal;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

uniform mat4 NM;

out vec3 V_Normal;
out vec4 V_WorldPos;
out vec4 V_EyeSpacePos;


void main()
{
    V_Normal = mat3(NM) * normal;

    vec4 worldPos = M * vec4(pos, 1); // 世界坐标系
    V_WorldPos = worldPos; 

    V_EyeSpacePos = V * worldPos;

    gl_Position=P*V* worldPos;
}
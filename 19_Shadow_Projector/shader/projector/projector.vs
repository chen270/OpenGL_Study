# version 330 core

// 逐像素渲染

attribute vec3 pos;
attribute vec2 texcoord;
attribute vec3 normal;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

uniform mat4 NM;
uniform mat4 U_ProjectNoTransScaleMatrix;
uniform mat4 U_ProjectorMatrix;

out vec3 V_Normal;
out vec4 V_WorldPos;
out vec2 V_Texcoord;
out vec4 V_ProjectCoord;
out vec4 V_PorjectSpaceFragPos;


void main()
{
    V_Normal = mat3(NM) * normal;

    V_WorldPos = M * vec4(pos, 1); // 世界坐标系

    V_Texcoord = texcoord;

    V_ProjectCoord = U_ProjectorMatrix * V_WorldPos;

    V_PorjectSpaceFragPos = U_ProjectNoTransScaleMatrix * V_WorldPos; 

    gl_Position=P*V*M*vec4(pos,1.0);
}
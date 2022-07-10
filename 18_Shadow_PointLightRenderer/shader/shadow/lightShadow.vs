# version 330 core

// 逐像素渲染

attribute vec3 pos;
attribute vec2 texcoord;
attribute vec3 normal;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

uniform mat4 NM;

out vec3 V_Normal;
out vec4 V_WorldPos;

uniform mat4 U_LightProjection;
uniform mat4 U_LightViewMatrix;
out vec4 V_LightSpaceFragPos;

void main()
{
    V_Normal = mat3(NM) * normal;

    V_WorldPos = M * vec4(pos, 1); // 世界坐标系

    V_LightSpaceFragPos = U_LightProjection * U_LightViewMatrix * M * vec4(pos,1.0);

    gl_Position=P*V*M*vec4(pos,1.0);
}
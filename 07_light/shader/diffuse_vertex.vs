# version 330 core

// 逐顶点渲染

attribute vec3 pos;
attribute vec2 texcoord;
attribute vec3 normal;

uniform mat4 M;
uniform mat4 V;
uniform mat4 P;

uniform mat4 NM;
uniform vec4 U_DiffuseLightColor;
uniform vec4 U_DiffuseMaterial; // 环境光反射的材质
uniform vec3 U_LightPos;

out vec4 V_DiffuseColor;


void main()
{
    // L vector
    vec3 L = U_LightPos;
    L = normalize(L);

    // n vector
    vec3 n = normalize(mat3(NM) * normal); // 局部坐标系转成世界坐标系

    float diffuseIntensity = max(0.0, dot(L,n));
    V_DiffuseColor = U_DiffuseLightColor * U_DiffuseMaterial * diffuseIntensity;

    gl_Position=P*V*M*vec4(pos,1.0);
}
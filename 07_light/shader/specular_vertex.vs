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
uniform vec3 U_EyePos;

uniform vec4 U_SpecularLightColor;
uniform vec4 U_SpecularMaterial; // 环境光反射的材质


out vec4 V_DiffuseColor;
out vec4 V_SpecularColor;

void main()
{
    // L vector, 物体表面指向光源
    vec3 L = U_LightPos;
    L = normalize(L);

    // n vector
    vec3 n = normalize(mat3(NM) * normal); // 局部坐标系转成世界坐标系

    float diffuseIntensity = max(0.0, dot(L,n));
    V_DiffuseColor = U_DiffuseLightColor * U_DiffuseMaterial * diffuseIntensity;

    // specular, -L 表示光源指向物体，即入射光线
    vec3 reflectDir = reflect(-L, n);
    reflectDir = normalize(reflectDir);
    vec4 worldPos = M * vec4(pos, 1); // 世界坐标系
    vec3 viewDir = U_EyePos - worldPos.xyz;
    viewDir = normalize(viewDir);
    V_SpecularColor = U_SpecularLightColor * U_SpecularMaterial * pow(max(0.0, dot(viewDir, reflectDir)), 128.0);

    gl_Position=P*V*M*vec4(pos,1.0);
}
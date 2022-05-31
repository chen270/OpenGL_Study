#version 330 core

in vec3 V_Normal;

uniform vec4 U_DiffuseLightColor;
uniform vec4 U_DiffuseMaterial; // 环境光反射的材质
uniform vec3 U_LightPos;

uniform vec4 U_AmbientLightColor;
uniform vec4 U_AmbientMaterial; // 环境光反射的材质

void main()
{
    // amibent 
    vec4 ambientColor = U_AmbientLightColor * U_AmbientMaterial; // 环境光

    // diffuse
    vec3 L = normalize(U_LightPos); // L vector
    vec3 n = normalize(V_Normal); // n vector
    float diffuseIntensity = max(0.0, dot(L,n));
    vec4 diffuseColor = U_DiffuseLightColor * U_DiffuseMaterial * diffuseIntensity;

    gl_FragColor = ambientColor + diffuseColor;
}
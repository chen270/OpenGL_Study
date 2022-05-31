#version 330 core

in vec4 V_DiffuseColor;

uniform vec4 U_AmbientLightColor;
uniform vec4 U_AmbientMaterial; // 环境光反射的材质

void main()
{
    // amibent 
    vec4 ambientColor = U_AmbientLightColor * U_AmbientMaterial; // 环境光

    // diffuse
    vec4 diffuseColor = V_DiffuseColor;

    gl_FragColor = ambientColor + diffuseColor;
}
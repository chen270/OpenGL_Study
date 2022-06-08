#version 330 core

in vec4 V_Color;
// in vec4 V_SpecularColor;

uniform vec4 U_AmbientLightColor;
uniform vec4 U_AmbientMaterial; // 环境光反射的材质

void main()
{
    // amibent 
    vec4 ambientColor = U_AmbientLightColor * U_AmbientMaterial; // 环境光

    // diffuse
    // vec4 diffuseColor = V_Color;

    // specular
    // vec4 specularColor = V_SpecularColor;

    gl_FragColor = ambientColor + V_Color;
}
#version 330 core

uniform vec4 U_AmbientLightColor;
uniform vec4 U_AmbientMaterial; // 环境光反射的材质

void main()
{
    //amibent 
    vec4 ambientColor = U_AmbientLightColor * U_AmbientMaterial; // 环境光
    gl_FragColor = ambientColor;
}
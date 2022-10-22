#version 330 core

uniform samplerCube U_MainTexture; // 立方体贴图的纹理采样器
in vec3 V_Texcoord;
out vec4 out_Color;

void main()
{
    out_Color = texture(U_MainTexture, V_Texcoord);
    //gl_FragColor = vec4(V_Texcoord+255.0, 1.0);
    //gl_FragColor = vec4(0.0,0.0,-0.5, 1.0);
}
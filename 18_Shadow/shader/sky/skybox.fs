# version 330 core

uniform samplerCube U_MainTexture; // 立方体贴图的纹理采样器
in vec3 V_Texcoord;

void main()
{
    gl_FragColor = texture(U_MainTexture, V_Texcoord);
}